# Argset
easy to use lightweight C library for adding start arguments

## usage

### Bool argument
```c
#include "src/argset/arg.h"
int main(int argc,char** argv){
        // ARGSET_DOUBLE_DASH_HELP changes "help" command to "--help"
        Argset *arg = argsetInit(argc,argv,ARGSET_DOUBLE_DASH_HELP);

        //Bool type stores how many times it was called
        // adds bool argument that has description "bool arg" and is called by "--bool" flag
        argsetAppendBool(arg,"--bool","bool arg");

        // adds "-b" alias for "--bool" arg
        // calling "-b" will be the same as calling "--bool"
        // alias can be added to any argument type
        // aliases does not show up in help command
        argsetAppendAlias(arg,"--bool","-b");

        // parses argc and argv
        argsetCall(arg);

        if(argsetGetBool(arg,"--bool"))
                printf("\"--bool\" argument was called\n");


        // frees memory allocated by argset
        argsetFree(arg);
}
```

### Func argument
```c
#include "src/argset/arg.h"
void function(void *arg){
        printf("function has been called\n");
}
int main(int argc,char** argv){
        Argset *arg = argsetInit(argc,argv,ARGSET_DOUBLE_DASH_HELP);

        // adds argument that when used calls function
        argsetAppendFunc(arg,"--func","function argument",function,NULL);

        argsetCall(arg);
        argsetFree(arg);
}
```
last argument in argsetAppendFunc is always passed to provided function as void*

### Iter argument
```c
#include "src/argset/arg.h"
void iterator(const char *txt, void *arg){
        printf("function has been called with argument: %s\n",txt);
}
int main(int argc,char** argv){
        Argset *arg = argsetInit(argc,argv,ARGSET_DOUBLE_DASH_HELP);

        // adds argument that when used calls function
        // 1 means user can't call "--iter" without more arguments
        // examples:
        //
        // valid:
        // ./program --iter a
        // ./program --iter --help
        // ./porgram --iter a --help
        //
        // invalid:
        // ./program --iter
        // ./porgram --iter --help a
        //
        // other commands does not trigger Iter arguments

        argsetAppendIter(arg,"--iter","iterator argument",iterator,NULL,1);

        argsetCall(arg);
        argsetFree(arg);
}
```

### Var argumets
```c
#include "src/argset/arg.h"
int main(int argc,char** argv){
        int a = 0;
        Argset *arg = argsetInit(argc,argv,ARGSET_DOUBLE_DASH_HELP);

        // there are more possible data types
        // char
   // TYPE_ARGSET_CHAR
        //
   // long
   // TYPE_ARGSET_LONG
        //
   // float
   // TYPE_ARGSET_FLOAT
        //
   // double
   // TYPE_ARGSET_DOUBLE
        //
   // const char*
   // TYPE_ARGSET_STR
        argsetAppendVar(arg,"--var","sets a varable",TYPE_ARGSET_INT,&a);

        argsetCall(arg);

        printf("a = %d\n");

        argsetFree(arg);
}
```

## Example

### simple calculator operated by passed arguments

```c
#include "src/argset/arg.h"
#include <stdio.h>
#include <stdlib.h>
void addNums(const char *txt, void *arg){
        float *calc = (float*)arg;
        *calc += atof(txt);
}
void subNums(const char *txt, void *arg){
        float *calc = (float*)arg;
        *calc -= atof(txt);
}
void mulNums(const char *txt, void *arg){
        float *calc = (float*)arg;
        *calc *= atof(txt);
}
void divNums(const char *txt, void *arg){
        float *calc = (float*)arg;
        float holder = atof(txt);
        if(holder == 0)
                return;
        *calc /= holder;
}
int main(int argc,char** argv){
        float calc = 0.0f;

        Argset *arg = argsetInit(argc,argv,ARGSET_DOUBLE_DASH_HELP);

        argsetAppendIter(arg,"--add",NULL,addNums,&calc,1);
        argsetAppendIter(arg,"--sub",NULL,subNums,&calc,1);
        argsetAppendIter(arg,"--mul",NULL,mulNums,&calc,1);
        argsetAppendIter(arg,"--div",NULL,divNums,&calc,1);

        argsetCall(arg);
        argsetFree(arg);

        printf("after all operations calc=%f\n",calc);
}
```
valid command:
```sh
./program --add 1 2 3 --sub 2 --mul 2 --div 4 2
```
translates to:\
((((0+1+2+3)-2)*2)/4)/2\
all arguments can be used more then one time so you can add at the end of this command for example "--add 10"
