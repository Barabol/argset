#ifndef __ARG_SET__
#define __ARG_SET__
#define __TRIE_NODES__ 128

/**
 * Enum: ArgsetFlags
 * \-----------------
 *
 * all flags that could be passed to
 *
 * argsetInit function
 *
 * if multiple flags are used thay sould be summed
 *
 * with '|' operator
 *
 */
typedef enum : char {
   // help command will not be generated
   ARGSET_NO_HELP = 1,
   // there will be no logs in terminal caused by Argset
   ARGSET_NO_TERM_LOGGING = 2,
} ArgsetFlags;

/**
 * Enum: ArgsetErrors
 * \------------------
 *
 * enum of all possible errors that
 *
 * could accure after function invocation
 *
 */
typedef enum : char {
   // no errors since last operation on argset
   ARGSET_NONE = 0,
   // last operation was not able to allocate memory
   ARGSET_OFM,
   // last operation was provided bad string
   ARGSET_BAD_STR,
} ArgsetErrors;

/**
 * Enum: ArgsetType
 * \----------------
 *
 * type of operation taht will be performed
 *
 * by argset when argument is called
 */
typedef enum : char { ARGSET_FUNC_CALL, ARGSET_ARG_LIST } ArgsetType;

/**
 * Struct: ArgsetListNode
 * \----------------------
 *
 * singular list node instance for argset
 *
 */
typedef struct __ArgsetNode__ {
   struct __ArgsetNode__ *next;
   const char *name;
   const char *desc;
} ArgsetListNode;

/**
 * Struct: ArgsetOper
 * \------------------
 *
 * structure of operation that should occure
 * when program is called with it's arg
 */
typedef struct {
	const char *name;
   union {
      struct {
         void (*ptr)(void *);
         void *arg;
      } fCall;
   };
   ArgsetType type;
} ArgsetOper;

/**
 * Structure: TrieNode
 * \-------------------
 *
 * base structure for trie tree
 *
 */
typedef struct __TrieNode__ {
   struct __TrieNode__ *children[__TRIE_NODES__];
   ArgsetOper *value;
} TrieNode;

/**
 * Struct: Argset
 *\--------------
 *
 *	main structude for argset lib
 *
 *	must be initialized
 *
 *	having more than one Argset initialized
 *	is not recomended
 *
 */
typedef struct {
   int argc;
   char **argv;
   TrieNode *tree;
   char flags;
   ArgsetErrors lastError;

   struct {
      ArgsetListNode *head;
      ArgsetListNode *tail;
      unsigned long len;
   } list;

} Argset;

/**
 * Function: argsetInit
 * \--------------------
 *
 *	function resposible for seting up argset
 *
 *	must have argc and argv passed to work properly
 *
 * returns NULL if not able to allocate memory
 * for structure
 */
Argset *argsetInit(int argc, char **argv, char flags);

/**
 * Function: agrsetAppend
 *	\--------------------
 *
 * adds argument that can be passed to program
 *
 * if arg is used function provided will be called
 *
 *	returns 0 if successfull
 *
 * description can be NULL if there is none
 */
int argsetAppendFunc(Argset *argset, const char *name, const char *desc,
                     void (*func)(void *), void *arg);

/**
 * Function: agrsetCall
 *	\--------------------
 *
 * checks argc and argv
 *
 * must be called once after last argset append
 *
 */
void argsetCall(Argset *argset);

/**
 * Function: agrsetFree
 *	\--------------------
 *
 * frees memory allocated by argset
 *
 */
void argsetFree(Argset *argset);

#endif
