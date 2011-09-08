#ifndef LIST_NAME_H
#error List requires LIST_NAME_H to be set. Function prefix will be
#error SNetLIST_NAME_H
#endif

#ifndef LIST_TYPE_NAME_H
#error List requires LIST_TYPE_NAME_H to be set.
#error Type will be snet_LIST_TYPE_NAME_H_list_t
#endif

#ifndef LIST_VAL_H
#error List requires LIST_VAL_H to be set. List content will be of type
#error LIST_VAL_H
#endif

/* !! CAUTION !!
 *
 * 1) The seemingly useless indirections using CONCAT and LIST(name) is to
 * ensure the macro's get fully expanded properly. Removing them (for example
 * defining snet_list_t directly using ## results in behaviour like
 * "LIST(LIST_NAME)" expanding to "snet_LIST_NAME_list_t" instead of containing
 * the *value* of LIST_NAME.
 *
 * 2) These macros are partially duplicated in list-template.c, keep in sync
 * under penalty of defenestration.
 *
 * 3) Similar macros are also used in map-template.c and map-template.h
 */
#define CONCAT(prefix, s1, s2, suffix)  prefix ## s1 ## s2 ## suffix
#define LIST(name)                      CONCAT(snet_, name, _list, _t)
#define LIST_FUNCTION(name, funName)    CONCAT(SNet, name, List, funName)
#define snet_list_t                     LIST(LIST_TYPE_NAME_H)

#define LIST_FOR_EACH(list, val) \
  for (int snet_list_ctr = 0; \
       (val = list->used \
              ? list->values[(list->start + snet_list_ctr) % list->size] \
              : val), \
       snet_list_ctr < list->used; \
       snet_list_ctr++)

#define LIST_ENUMERATE(list, index, val) \
  for (index = 0; \
       (val = list->used \
              ? list->values[(list->start + index) % list->size] \
              : val), \
        index < list->used; index++)

#define LIST_ZIP_EACH(list1, list2, val1, val2) \
  for (int snet_list_ctr = 0; \
       (val1 = list1->used \
               ? list1->values[(list1->start + snet_list_ctr) % list1->size] \
               : val1), \
       (val2 = list2->used \
               ? list2->values[(list2->start + snet_list_ctr) % list2->size] \
               : val2), \
       snet_list_ctr < list1->used && snet_list_ctr < list2->used; \
       snet_list_ctr++)

#define LIST_ZIP_ENUMERATE(list1, list2, index, val1, val2) \
  for (index = 0; \
       (val1 = list1->used \
               ? list1->values[(list1->start + index) % list1->size] \
               : val1), \
       (val2 = list2->used \
               ? list2->values[(list2->start + index) % list2->size] \
               : val2),\
       index < list1->used && index < list2->used; \
       index++)

#define LIST_DEQUEUE_EACH(list, val) \
  for (int size = 0; \
       size = list->used, \
       val = size != 0 ? list->values[list->start] : val, \
       list->start = size ? (list->start + 1) % list->size : list->start, \
       list->used = size ? list->used - 1 : list->used, \
       size != 0;)

typedef struct snet_list_t {
  int size, used, start;
  LIST_VAL_H *values;
} snet_list_t;

snet_list_t *LIST_FUNCTION(LIST_NAME_H, Create)(int size, ...);
snet_list_t *LIST_FUNCTION(LIST_NAME_H, Copy)(snet_list_t *list);
void LIST_FUNCTION(LIST_NAME_H, Destroy)(snet_list_t *list);

snet_list_t *LIST_FUNCTION(LIST_NAME_H, DeepCopy)(
        snet_list_t *list,
        LIST_VAL_H (*copyfun)(LIST_VAL_H));

int LIST_FUNCTION(LIST_NAME_H, Length)(snet_list_t *list);

void LIST_FUNCTION(LIST_NAME_H, AppendStart)(snet_list_t *list, LIST_VAL_H val);
void LIST_FUNCTION(LIST_NAME_H, AppendEnd)(snet_list_t *list, LIST_VAL_H val);
LIST_VAL_H LIST_FUNCTION(LIST_NAME_H, PopStart)(snet_list_t *list);
LIST_VAL_H LIST_FUNCTION(LIST_NAME_H, PopEnd)(snet_list_t *list);

bool LIST_FUNCTION(LIST_NAME_H, Contains)(snet_list_t *list, LIST_VAL_H val);

LIST_VAL_H LIST_FUNCTION(LIST_NAME_H, Get)(snet_list_t *list, int i);
LIST_VAL_H LIST_FUNCTION(LIST_NAME_H, Remove)(snet_list_t *list, int i);

void LIST_FUNCTION(LIST_NAME_H, Serialise)(snet_list_t *list,
                                       void (*serialiseInts)(int, int*),
                                       void (*serialiseValues)(int, LIST_VAL_H*));

void LIST_FUNCTION(LIST_NAME_H, Deserialise)(snet_list_t *list,
                                       void (*deserialiseInts)(int, int*),
                                       void (*deserialiseValues)(int, LIST_VAL_H*));

#undef snet_list_t
#undef LIST_FUNCTION
#undef LIST
#undef CONCAT
