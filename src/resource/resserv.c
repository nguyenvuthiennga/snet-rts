#include <hwloc.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "resdefs.h"

typedef enum res_kind res_kind_t;
typedef struct res res_t;

enum res_kind {
  System,
  Machine,
  Node,
  Socket,
  Cache,
  Core,
  Proc,
  Other,
};

struct res {
  int           depth;
  res_kind_t    kind;
  int           logical;
  int           physical;
  int           cache_level;
  struct res   *parent;
  struct res  **children;
  int           num_children;
  int           max_children;
  int           first_pu;
  int           last_pu;
};

static res_t *root;

char* res_topo_string(res_t* obj, char* str, int len, int *size)
{
  if (!obj) {
    obj = root;
  }
  assert(str);
  assert(*size >= 100);
  assert(len >= 0 && len < *size);
  assert(str[len] == '\0');

  if (*size - len < 1024) {
    str = xrealloc(str, *size + 1024);
    *size += 1024;
  }
  snprintf(str + len, *size - len,
           "%*s{ kind %s depth %d logical %d ",
           obj->depth * 2, "",
           res_kind_string(obj->kind), obj->depth, obj->logical);
  len += strlen(str + len);

  if (obj->kind == Proc) {
    snprintf(str + len, *size - len, "physical %d ", obj->physical);
    len += strlen(str + len);
  }
  if (obj->kind == Cache) {
    snprintf(str + len, *size - len, "level %d ", obj->cache_level);
    len += strlen(str + len);
  }
  if (obj->num_children > 0) {
    int i;
    snprintf(str + len, *size - len, "children %d \n", obj->num_children);
    len += strlen(str + len);
    for (i = 0; i < obj->num_children; ++i) {
      str = res_topo_string(obj->children[i], str, len, size);
      len += strlen(str + len);
    }
    snprintf(str + len, *size - len,
             "%*s} \n", obj->depth * 2, "");
  } else {
    snprintf(str + len, *size - len, "} \n");
  }

  return str;
}

static void res_relate(res_t *parent, res_t *child)
{
  if (parent->num_children == parent->max_children) {
    parent->max_children = parent->max_children
                         ? 2 * parent->max_children
                         : 1;
    parent->children = xrealloc(parent->children,
                                parent->max_children * sizeof(res_t *));
  }
  assert(parent->num_children < parent->max_children);
  parent->children[parent->num_children++] = child;
}

static res_t* res_add(
  res_t *parent,
  res_kind_t kind,
  int topo_depth,
  int logical_index,
  int physical_index,
  int cache_level)
{
  res_t *res = xnew(res_t);
  res->depth = topo_depth;
  res->kind = kind;
  res->logical = logical_index;
  res->physical = physical_index;
  res->cache_level = cache_level;
  res->parent = parent;
  res->children = 0;
  res->num_children = 0;
  res->max_children = 0;
  res->first_pu = -1;
  res->last_pu = -1;
  if (parent == NULL) {
    assert(root == NULL);
    root = res;
  } else {
    res_relate(parent, res);
  }
  return res;
}

static res_kind_t res_type_to_kind(hwloc_obj_type_t type)
{
  res_kind_t kind = Other;
  switch (type) {
    case HWLOC_OBJ_SYSTEM:  kind = System;  break;
    case HWLOC_OBJ_MACHINE: kind = Machine; break;
    case HWLOC_OBJ_NODE:    kind = Node;    break;
    case HWLOC_OBJ_SOCKET:  kind = Socket;  break;
    case HWLOC_OBJ_CACHE:   kind = Cache;   break;
    case HWLOC_OBJ_CORE:    kind = Core;    break;
    case HWLOC_OBJ_PU:      kind = Proc;    break;
    default:
      res_error("Invalid type %s\n", hwloc_obj_type_string(type));
  }
  return kind;
}

static void traverse_topo(
  hwloc_topology_t topo,
  hwloc_obj_t obj,
  int depth,
  res_t* parent)
{
  int i;
  res_kind_t kind;
  res_t* res;

  switch (obj->type) {
    case HWLOC_OBJ_SYSTEM:
    case HWLOC_OBJ_MACHINE:
    case HWLOC_OBJ_NODE:
    case HWLOC_OBJ_SOCKET:
    case HWLOC_OBJ_CACHE:
    case HWLOC_OBJ_CORE:
    case HWLOC_OBJ_PU:
      kind = res_type_to_kind(obj->type);
      break;
    default:
      res_warn("Ignoring hwloc object %s at depth %d.\n",
                hwloc_obj_type_string(obj->type), depth);
      return;
  }
  res = res_add(parent, kind, depth, obj->logical_index, obj->os_index,
                (kind == Cache && obj->attr) ?  obj->attr->cache.depth : 0);
  if (res->kind == Proc) {
    assert(obj->arity == 0);
    res->first_pu = res->last_pu = obj->logical_index;
  }
  else {
    for (i = 0; i < obj->arity; i++) {
      traverse_topo(topo, obj->children[i], depth + 1, res);
    }
    for (i = 0; i < res->num_children; i++) {
      if (i == 0) {
        res->first_pu = res->children[i]->first_pu;
        res->last_pu = res->children[i]->last_pu;
      } else {
        if (res->children[i]->first_pu != res->last_pu + 1) {
          res_warn("Gap at depth %d, child %d, %d -> %d\n",
                   depth, i, res->last_pu, res->children[i]->first_pu);
        }
        res->last_pu = res->children[i]->last_pu;
      }
    }
  }
  res_info("Depth %d, %-7s, range %d -> %d\n",
           depth, res_kind_string(res->kind),
           res->first_pu, res->last_pu);
}

void res_hw_init(void)
{
  hwloc_topology_t topo;

  /* Allocate and initialize topology object. */
  hwloc_topology_init(&topo);

  /* Perform the topology detection. */
  hwloc_topology_load(topo);

  traverse_topo(topo, hwloc_get_root_obj(topo), 0, root);

  hwloc_topology_destroy(topo);
}

const char *res_kind_string(int kind)
{
  return
#define NAME(x) #x
#define KIND(k) kind == k ? NAME(k) :
  KIND(System)
  KIND(Machine)
  KIND(Node)
  KIND(Socket)
  KIND(Cache)
  KIND(Core)
  KIND(Proc)
  KIND(Other)
  "Unknown";
}

