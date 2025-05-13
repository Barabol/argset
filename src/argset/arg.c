#include "arg.h"
#include <stdio.h>
#include <stdlib.h>

void helpCommand(void *vNode) {
   if (vNode == NULL)
      return;

   Argset *arg = (Argset *)vNode;
   ArgsetListNode *node = arg->list.tail;

   printf("\n   \033[1m%-22s %-22s\033[0m\n\n", "command", "description");
   while (node != NULL) {
      printf("   %-22s %-22s\n", node->name, node->desc);
      node = node->next;
   }
   puts("");
}

int AStrieInsert(ArgsetTrieNode *node, const char *key, ArgsetOper *value,
                 char useLower) {
   if (node == NULL)
      return -1;
   char used;
   for (; *key; key++) {
      used = (useLower && *key >= 'A' && *key <= 'Z') ? *key + 32 : *key;
      if (node->children[used] == NULL) {
         node->children[used] =
             (ArgsetTrieNode *)malloc(sizeof(ArgsetTrieNode));

         if (node->children[used] == NULL)
            return 1;

         for (int x = 0; x < __TRIE_NODES__; x++)
            node->children[used]->children[x] = NULL;
      }
      node = node->children[used];
      node->value = NULL;
   }
   node->value = value;
   return 0;
}

ArgsetOper *AStrieValue(ArgsetTrieNode *node, const char *key, char useLower) {
   if (node == NULL)
      return NULL;
   for (; *key; key++) {
      node = node->children[(useLower && *key >= 'A' && *key <= 'Z') ? *key + 32
                                                                     : *key];
      if (!node)
         return NULL;
   }
   return node->value;
}

void trieFree(ArgsetTrieNode *node) {
   if (node == NULL)
      return;

   if (node->value != NULL) {
      if (node->value->isAlias != 0)
         node->value->isAlias--;
      else
         free(node->value);
   }

   for (int x = 0; x < __TRIE_NODES__; x++) {
      if (node->children[x] == NULL)
         continue;

      trieFree(node->children[x]);
      free(node->children[x]);
   }
}

Argset *argsetInit(int argc, char **argv, char flags) {
   if (argv == NULL)
      return NULL;

   Argset *argset = NULL;
   argset = (Argset *)malloc(sizeof(Argset));

   if (argset == NULL) {
      if (~flags & ARGSET_NO_TERM_LOGGING)
         puts("unable to allocate memory");
      return NULL;
   }

   ArgsetTrieNode *tree = NULL;
   tree = (ArgsetTrieNode *)malloc(sizeof(ArgsetTrieNode));

   tree->value = NULL;
   for (int x = 0; x < __TRIE_NODES__; x++)
      tree->children[x] = NULL;

   if (tree == NULL) {
      if (~flags & ARGSET_NO_TERM_LOGGING)
         puts("unable to allocate memory");
      free(argset);
      return NULL;
   }

   argset->argc = argc;
   argset->argv = argv;
   argset->flags = flags;
   argset->tree = tree;
   argset->list.len = 0;
   argset->list.head = NULL;
   argset->list.tail = NULL;
   argset->lastError = ARGSET_NONE;

   if (~flags & ARGSET_NO_HELP)
      argsetAppendFunc(argset,
                       flags & ARGSET_DOUBLE_DASH_HELP ? "--help" : "help",
                       "displays all commands", helpCommand, argset);

   return argset;
}

int listAdd(Argset *arg, const char *name, const char *desc) {
   ArgsetListNode *node;
   node = (ArgsetListNode *)malloc(sizeof(ArgsetListNode));
   if (node == NULL) {
      arg->lastError = ARGSET_OFM;
      if (~arg->flags & ARGSET_NO_TERM_LOGGING)
         puts("unable to allocate memory");
      return 1;
   }

   node->next = NULL;
   node->name = name;
   node->desc = desc == NULL ? "" : desc;

   if (arg->list.tail == NULL) {
      arg->list.len = 1;
      arg->list.head = node;
      arg->list.tail = node;
      return 0;
   }
   arg->list.len++;
   arg->list.head->next = node;
   arg->list.head = node;
   return 0;
}

int argsetAppendFunc(Argset *argset, const char *name, const char *desc,
                     void (*func)(void *), void *arg) {
   if (argset == NULL)
      return 1;
   if (name == NULL) {
      argset->lastError = ARGSET_BAD_STR;
      if (~argset->flags & ARGSET_NO_TERM_LOGGING)
         puts("bad name was provided");
      return 1;
   }
   ArgsetOper *oper = NULL;
   oper = (ArgsetOper *)malloc(sizeof(ArgsetOper));
   if (oper == NULL) {
      argset->lastError = ARGSET_OFM;
      if (~argset->flags & ARGSET_NO_TERM_LOGGING)
         puts("unable to allocate memory");
      return 1;
   }

   AStrieInsert(argset->tree, name, oper,
                argset->flags & ARGSET_NO_CASEMATCH ? 1 : 0);
   if (~argset->flags & ARGSET_NO_HELP)
      listAdd(argset, name, desc);

   oper->name = name;
   oper->type = ARGSET_FUNC_CALL;
   oper->fCall.ptr = func;
   oper->fCall.arg = arg;
   oper->isAlias = 0;
   return 0;
}

void varCall(ArgsetDataType type, void *var, const char *txt) {
   printf("str: \"%s\"\n", txt);
   switch (type) {
   case TYPE_ARGSET_CHAR: {
      char *v = (char *)var;
      *v = atoi(txt) & 0xFF;
      if (txt[0] != 0 && txt[1] == 0)
         *v = txt[0];
   } break;
   case TYPE_ARGSET_INT: {
      int *v = (int *)var;
      *v = atoi(txt);
   } break;
   case TYPE_ARGSET_LONG: {
      long *v = (long *)var;
      *v = atol(txt);
   } break;
   case TYPE_ARGSET_FLOAT: {
      float *v = (float *)var;
      *v = atof(txt);
   } break;
   case TYPE_ARGSET_DOUBLE: {
      double *v = (double *)var;
      *v = atof(txt);
   } break;
   case TYPE_ARGSET_STR: {
      const char *v = (const char *)var;
      v = txt;
   } break;
   }
}

int argsetGetBool(Argset *argset, const char *name) {
   if (argset == NULL || name == NULL)
      return 1;

   ArgsetOper *oper = AStrieValue(argset->tree, name,
                                  argset->flags & ARGSET_NO_CASEMATCH ? 1 : 0);

   if (oper == NULL)
      return -1;

   if (oper->type == ARGSET_BOOL)
      return oper->bCall.calls;

   return -1;
}

int argsetAppendAlias(Argset *argset, const char *name, const char *alias) {
   if (argset == NULL || name == NULL || alias == NULL)
      return 1;

   ArgsetOper *oper = AStrieValue(argset->tree, name,
                                  argset->flags & ARGSET_NO_CASEMATCH ? 1 : 0);

   oper->isAlias++;
   if (oper == NULL)
      return -1;

   AStrieInsert(argset->tree, alias, oper,
                argset->flags & ARGSET_NO_CASEMATCH ? 1 : 0);
   return 0;
}

int argsetAppendBool(Argset *argset, const char *name, const char *desc) {
   if (argset == NULL)
      return 1;
   if (name == NULL) {
      argset->lastError = ARGSET_BAD_STR;
      if (~argset->flags & ARGSET_NO_TERM_LOGGING)
         puts("bad name was provided");
      return 1;
   }
   ArgsetOper *oper = NULL;
   oper = (ArgsetOper *)malloc(sizeof(ArgsetOper));
   if (oper == NULL) {
      argset->lastError = ARGSET_OFM;
      if (~argset->flags & ARGSET_NO_TERM_LOGGING)
         puts("unable to allocate memory");
      return 1;
   }

   AStrieInsert(argset->tree, name, oper,
                argset->flags & ARGSET_NO_CASEMATCH ? 1 : 0);
   if (~argset->flags & ARGSET_NO_HELP)
      listAdd(argset, name, desc);

   oper->name = name;
   oper->isAlias = 0;
   oper->type = ARGSET_BOOL;
   oper->bCall.calls = 0;
   return 0;
}

int argsetAppendVar(Argset *argset, const char *name, const char *desc,
                    ArgsetDataType type, void *arg) {
   if (argset == NULL)
      return 1;
   if (name == NULL) {
      argset->lastError = ARGSET_BAD_STR;
      if (~argset->flags & ARGSET_NO_TERM_LOGGING)
         puts("bad name was provided");
      return 1;
   }
   ArgsetOper *oper = NULL;
   oper = (ArgsetOper *)malloc(sizeof(ArgsetOper));
   if (oper == NULL) {
      argset->lastError = ARGSET_OFM;
      if (~argset->flags & ARGSET_NO_TERM_LOGGING)
         puts("unable to allocate memory");
      return 1;
   }

   AStrieInsert(argset->tree, name, oper,
                argset->flags & ARGSET_NO_CASEMATCH ? 1 : 0);
   if (~argset->flags & ARGSET_NO_HELP)
      listAdd(argset, name, desc);

   oper->name = name;
   oper->type = ARGSET_VARABLE;
   oper->vCall.arg = arg;
   oper->vCall.type = type;
   oper->isAlias = 0;
   return 0;
}

int argsetAppendIter(Argset *argset, const char *name, const char *desc,
                     void (*func)(const char *, void *), void *arg,
                     int minArgs) {
   if (argset == NULL)
      return 1;
   ArgsetOper *oper = NULL;
   oper = (ArgsetOper *)malloc(sizeof(ArgsetOper));
   if (oper == NULL) {
      argset->lastError = ARGSET_OFM;
      if (~argset->flags & ARGSET_NO_TERM_LOGGING)
         puts("unable to allocate memory");
      return 1;
   }

   if (name == NULL)
      argset->tree->value = oper;
   else
      AStrieInsert(argset->tree, name, oper,
                   argset->flags & ARGSET_NO_CASEMATCH ? 1 : 0);

   if (name != NULL && ~argset->flags & ARGSET_NO_HELP)
      listAdd(argset, name, desc);

   oper->name = name;
   oper->type = ARGSET_ARG_LIST;
   oper->lCall.ptr = func;
   oper->lCall.arg = arg;
   oper->lCall.minArgs = minArgs;
   oper->isAlias = 0;
   return 0;
}

void argsetCall(Argset *argset) {
   if (argset == NULL)
      return;

   ArgsetOper *oper = NULL;
   ArgsetOper *last = NULL;
   int reqArgs = 0;

   if (~argset->flags & ARGSET_NO_CALL_CHECK)
      for (int x = 1; x < argset->argc; x++) {
         oper = AStrieValue(argset->tree, argset->argv[x],
                            argset->flags & ARGSET_NO_CASEMATCH ? 1 : 0);
         if (oper != NULL) {
            if (reqArgs != 0) {
               if (~argset->flags & ARGSET_NO_TERM_LOGGING)
                  puts("invalid arg");
               argset->lastError = ARGSET_BAD_ARGS;
               return;
            }
            if (oper->type == ARGSET_ARG_LIST) {
               reqArgs = oper->lCall.minArgs;
               last = oper;
               continue;
            } else if (oper->type == ARGSET_BOOL)
               continue;
            last = NULL;
            if (oper->type == ARGSET_FUNC_CALL)
               continue;
            else if (oper->type == ARGSET_VARABLE) {
               x++;
               if (x >= argset->argc) {
                  if (~argset->flags & ARGSET_NO_TERM_LOGGING)
                     puts("invalid arg");
                  argset->lastError = ARGSET_BAD_ARGS;
                  return;
               }
               continue;
            }
         } else if (last != NULL) {
            if (reqArgs > 0)
               reqArgs--;
            continue;
         } else {
            if (argset->tree->value != NULL) {
               continue;
            }
            if (~argset->flags & ARGSET_NO_TERM_LOGGING)
               puts("invalid arg");
            argset->lastError = ARGSET_BAD_ARGS;
            return;
         }
      }
   for (int x = 1; x < argset->argc; x++) {
      oper = AStrieValue(argset->tree, argset->argv[x],
                         argset->flags & ARGSET_NO_CASEMATCH ? 1 : 0);
      if (oper != NULL) {
         if (reqArgs != 0) {
            if (~argset->flags & ARGSET_NO_TERM_LOGGING)
               puts("invalid arg");
            argset->lastError = ARGSET_BAD_ARGS;
            return;
         }
         if (oper->type == ARGSET_ARG_LIST) {
            last = oper;
            continue;
         } else if (oper->type == ARGSET_BOOL) {
            oper->bCall.calls++;
            continue;
         }
         last = NULL;
         if (oper->type == ARGSET_FUNC_CALL)
            oper->fCall.ptr(oper->fCall.arg);
         if (oper->type == ARGSET_VARABLE) {
            x++;
            if (x >= argset->argc) {
               if (~argset->flags & ARGSET_NO_TERM_LOGGING)
                  puts("invalid arg");
               argset->lastError = ARGSET_BAD_ARGS;
               return;
            }
            varCall(oper->vCall.type, oper->vCall.arg, argset->argv[x]);
            continue;
         }
      } else if (last != NULL) {
         if (reqArgs > 0)
            reqArgs--;
         last->lCall.ptr(argset->argv[x], last->lCall.arg);
      } else {
         if (argset->tree->value != NULL) {
            oper = argset->tree->value;
            oper->lCall.ptr(argset->argv[x], oper->lCall.arg);
            continue;
         }
         if (~argset->flags & ARGSET_NO_TERM_LOGGING)
            puts("invalid arg");
         argset->lastError = ARGSET_BAD_ARGS;
         break;
      }
   }
}

void argsetFree(Argset *argset) {
   if (argset == NULL)
      return;

   trieFree(argset->tree);
   free(argset->tree);

   ArgsetListNode *node = argset->list.tail;
   ArgsetListNode *holder;
   while (node != NULL) {
      holder = node;
      node = node->next;
      free(holder);
   }
   free(argset);
}
