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

int trieInsert(TrieNode *node, const char *key, ArgsetOper *value) {
   if (node == NULL)
      return -1;
   for (; *key; key++) {
      if (node->children[*key] == NULL) {
         node->children[*key] = (TrieNode *)malloc(sizeof(TrieNode));

         if (node->children[*key] == NULL)
            return 1;

         for (int x = 0; x < __TRIE_NODES__; x++)
            node->children[*key]->children[x] = NULL;
      }
      node = node->children[*key];
      node->value = NULL;
   }
   node->value = value;
   return 0;
}

ArgsetOper *trieValue(TrieNode *node, const char *key) {
   if (node == NULL)
      return NULL;
   for (; *key; key++) {
      node = node->children[*key];
      if (!node)
         return NULL;
   }
   return node->value;
}

void trieFree(TrieNode *node) {
   if (node == NULL)
      return;

   if (node->value != NULL)
      free(node->value);

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

   TrieNode *tree = NULL;
   tree = (TrieNode *)malloc(sizeof(TrieNode));

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
      argsetAppendFunc(argset, "help", "displays all commands", helpCommand,
                       argset);

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

   trieInsert(argset->tree, name, oper);
   if (~argset->flags & ARGSET_NO_HELP)
      listAdd(argset, name, desc);

   oper->name = name;
   oper->type = ARGSET_FUNC_CALL;
   oper->fCall.ptr = func;
   oper->fCall.arg = arg;
   return 0;
}

int argsetAppendIter(Argset *argset, const char *name, const char *desc,
                     void (*func)(const char *, void *), void *arg) {
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

   trieInsert(argset->tree, name, oper);
   if (~argset->flags & ARGSET_NO_HELP)
      listAdd(argset, name, desc);

   oper->name = name;
   oper->type = ARGSET_ARG_LIST;
   oper->lCall.ptr = func;
   oper->lCall.arg = arg;
   return 0;
}

void argsetCall(Argset *argset) {
   if (argset == NULL)
      return;

   ArgsetOper *oper = NULL;
   ArgsetOper *last = NULL;
   if (~argset->flags & ARGSET_NO_CALL_CHECK)
      for (int x = 1; x < argset->argc; x++) {
         oper = trieValue(argset->tree, argset->argv[x]);
         if (oper != NULL) {
            if (oper->type == ARGSET_ARG_LIST) {
               last = oper;
               continue;
            }
            last = NULL;
            if (oper->type == ARGSET_FUNC_CALL)
               continue;
         } else if (last != NULL) {
            continue;
         } else {
            if (~argset->flags & ARGSET_NO_TERM_LOGGING)
               puts("invalid arg");
            return;
         }
      }
   for (int x = 1; x < argset->argc; x++) {
      oper = trieValue(argset->tree, argset->argv[x]);
      if (oper != NULL) {
         if (oper->type == ARGSET_ARG_LIST) {
            last = oper;
            continue;
         }
         last = NULL;
         if (oper->type == ARGSET_FUNC_CALL)
            oper->fCall.ptr(oper->fCall.arg);
      } else if (last != NULL) {
         last->lCall.ptr(argset->argv[x], last->lCall.arg);
      } else {
         if (~argset->flags & ARGSET_NO_TERM_LOGGING)
            puts("invalid arg");
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
