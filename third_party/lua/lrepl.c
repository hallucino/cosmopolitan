#define lua_c
#include "libc/calls/calls.h"
#include "libc/calls/sigbits.h"
#include "libc/log/check.h"
#include "libc/runtime/gc.h"
#include "libc/runtime/runtime.h"
#include "libc/sysv/consts/sa.h"
#include "libc/x/x.h"
#include "third_party/linenoise/linenoise.h"
#include "third_party/lua/lauxlib.h"
#include "third_party/lua/lprefix.h"
#include "third_party/lua/lua.h"
#include "third_party/lua/lualib.h"
// clang-format off

static lua_State *globalL;
static const char *g_progname;

/*
** {==================================================================
** Read-Eval-Print Loop (REPL)
** ===================================================================
*/

#if !defined(LUA_PROMPT)
#define LUA_PROMPT		">: "
#define LUA_PROMPT2		">>: "
#endif

#if !defined(LUA_MAXINPUT)
#define LUA_MAXINPUT		512
#endif

static void lua_readline_addcompletion(linenoiseCompletions *c, char *s) {
  char **p = c->cvec;
  size_t n = c->len + 1;
  if ((p = realloc(p, n * sizeof(*p)))) {
    p[n - 1] = s;
    c->cvec = p;
    c->len = n;
  }
}

void lua_readline_completions(const char *p, linenoiseCompletions *c) {
  lua_State *L;
  const char *name;
  L = globalL;
  lua_pushglobaltable(L);
  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    name = lua_tostring(L, -2);
    if (startswithi(name, p)) {
      lua_readline_addcompletion(c, strdup(name));
    }
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
}

char *lua_readline_hint(const char *p, const char **ansi1, const char **ansi2) {
  char *h = 0;
  linenoiseCompletions c = {0};
  lua_readline_completions(p, &c);
  if (c.len == 1) h = strdup(c.cvec[0] + strlen(p));
  linenoiseFreeCompletions(&c);
  return h;
}

static void lua_freeline (lua_State *L, char *b) {
  free(b);
}

/*
** Return the string to be used as a prompt by the interpreter. Leave
** the string (or nil, if using the default value) on the stack, to keep
** it anchored.
*/
static const char *get_prompt (lua_State *L, int firstline) {
  if (lua_getglobal(L, firstline ? "_PROMPT" : "_PROMPT2") == LUA_TNIL)
    return (firstline ? LUA_PROMPT : LUA_PROMPT2);  /* use the default */
  else {  /* apply 'tostring' over the value */
    const char *p = luaL_tolstring(L, -1, NULL);
    lua_remove(L, -2);  /* remove original value */
    return p;
  }
}

/* mark in error messages for incomplete statements */
#define EOFMARK		"<eof>"
#define marklen		(sizeof(EOFMARK)/sizeof(char) - 1)


/*
** Check whether 'status' signals a syntax error and the error
** message at the top of the stack ends with the above mark for
** incomplete statements.
*/
static int incomplete (lua_State *L, int status) {
  if (status == LUA_ERRSYNTAX) {
    size_t lmsg;
    const char *msg = lua_tolstring(L, -1, &lmsg);
    if (lmsg >= marklen && strcmp(msg + lmsg - marklen, EOFMARK) == 0) {
      lua_pop(L, 1);
      return 1;
    }
  }
  return 0;  /* else... */
}


/*
** Prompt the user, read a line, and push it into the Lua stack.
*/
static int pushline (lua_State *L, int firstline) {
  char *b;
  size_t l;
  globalL = L;
  const char *prmt = get_prompt(L, firstline);
  if (!(b = linenoiseWithHistory(prmt, g_progname)))
    return 0;  /* no input (prompt will be popped by caller) */
  lua_pop(L, 1);  /* remove prompt */
  l = strlen(b);
  if (l > 0 && b[l-1] == '\n')  /* line ends with newline? */
    b[--l] = '\0';  /* remove it */
  if (firstline && b[0] == '=')  /* for compatibility with 5.2, ... */
    lua_pushfstring(L, "return %s", b + 1);  /* change '=' to 'return' */
  else
    lua_pushlstring(L, b, l);
  lua_freeline(L, b);
  return 1;
}


/*
** Try to compile line on the stack as 'return <line>;'; on return, stack
** has either compiled chunk or original line (if compilation failed).
*/
static int addreturn (lua_State *L) {
  const char *line = lua_tostring(L, -1);  /* original line */
  const char *retline = lua_pushfstring(L, "return %s;", line);
  int status = luaL_loadbuffer(L, retline, strlen(retline), "=stdin");
  if (status == LUA_OK) {
    lua_remove(L, -2);  /* remove modified line */
  } else {
    lua_pop(L, 2);  /* pop result from 'luaL_loadbuffer' and modified line */
  }
  return status;
}


/*
** Hook set by signal function to stop the interpreter.
*/
static void lstop (lua_State *L, lua_Debug *ar) {
  (void)ar;  /* unused arg. */
  lua_sethook(L, NULL, 0, 0);  /* reset hook */
  luaL_error(L, "interrupted!");
}


/*
** Read multiple lines until a complete Lua statement
*/
static int multiline (lua_State *L) {
  for (;;) {  /* repeat until gets a complete statement */
    size_t len;
    const char *line = lua_tolstring(L, 1, &len);  /* get what it has */
    int status = luaL_loadbuffer(L, line, len, "=stdin");  /* try it */
    if (!incomplete(L, status) || !pushline(L, 0)) {
      return status;  /* cannot or should not try to add continuation line */
    }
    lua_pushliteral(L, "\n");  /* add newline... */
    lua_insert(L, -2);  /* ...between the two lines */
    lua_concat(L, 3);  /* join them */
  }
}


void lua_initrepl(const char *progname) {
  g_progname = progname;
  linenoiseSetCompletionCallback(lua_readline_completions);
  linenoiseSetHintsCallback(lua_readline_hint);
  linenoiseSetFreeHintsCallback(free);
}


/*
** Read a line and try to load (compile) it first as an expression (by
** adding "return " in front of it) and second as a statement. Return
** the final status of load/call with the resulting function (if any)
** in the top of the stack.
*/
int lua_loadline (lua_State *L) {
  int status;
  lua_settop(L, 0);
  if (!pushline(L, 1))
    return -1;  /* no input */
  if ((status = addreturn(L)) != LUA_OK)  /* 'return ...' did not work? */
    status = multiline(L);  /* try as command, maybe with continuation lines */
  lua_remove(L, 1);  /* remove line from the stack */
  lua_assert(lua_gettop(L) == 1);
  return status;
}


/*
** Function to be called at a C signal. Because a C signal cannot
** just change a Lua state (as there is no proper synchronization),
** this function only sets a hook that, when called, will stop the
** interpreter.
*/
static void laction (int i) {
  int flag = LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE | LUA_MASKCOUNT;
  lua_sethook(globalL, lstop, flag, 1);
}


/*
** Message handler used to run all chunks
*/
static int msghandler (lua_State *L) {
  const char *msg = lua_tostring(L, 1);
  if (msg == NULL) {  /* is error object not a string? */
    if (luaL_callmeta(L, 1, "__tostring") &&  /* does it have a metamethod */
        lua_type(L, -1) == LUA_TSTRING)  /* that produces a string? */
      return 1;  /* that is the message */
    else
      msg = lua_pushfstring(L, "(error object is a %s value)",
                               luaL_typename(L, 1));
  }
  luaL_traceback(L, L, msg, 1);  /* append a standard traceback */
  return 1;  /* return the traceback */
}


/*
** Interface to 'lua_pcall', which sets appropriate message function
** and C-signal handler. Used to run all chunks.
*/
int lua_runchunk (lua_State *L, int narg, int nres) {
  struct sigaction sa, saold;
  int status;
  int base = lua_gettop(L) - narg;  /* function index */
  lua_pushcfunction(L, msghandler);  /* push message handler */
  lua_insert(L, base);  /* put it under function and args */
  globalL = L;  /* to be available to 'laction' */
  sa.sa_flags = SA_RESETHAND; /* if another int happens, terminate */
  sa.sa_handler = laction;
  sigemptyset(&sa.sa_mask); /* do not mask any signal */
  sigaction(SIGINT, &sa, &saold);
  status = lua_pcall(L, narg, nres, base);
  sigaction(SIGINT, &saold, 0); /* restore C-signal handler */
  lua_remove(L, base);  /* remove message handler from the stack */
  return status;
}


/*
** Prints an error message, adding the program name in front of it
** (if present)
*/
void lua_l_message (const char *pname, const char *msg) {
  if (pname) lua_writestringerror("%s: ", pname);
  lua_writestringerror("%s\n", msg);
}


/*
** Prints (calling the Lua 'print' function) any values on the stack
*/
void lua_l_print (lua_State *L) {
  int n = lua_gettop(L);
  if (n > 0) {  /* any result to be printed? */
    luaL_checkstack(L, LUA_MINSTACK, "too many results to print");
    lua_getglobal(L, "print");
    lua_insert(L, 1);
    if (lua_pcall(L, n, 0, 0) != LUA_OK)
      lua_l_message(g_progname, lua_pushfstring(L, "error calling 'print' (%s)",
                                                lua_tostring(L, -1)));
  }
}


/*
** Check whether 'status' is not OK and, if so, prints the error
** message on the top of the stack. It assumes that the error object
** is a string, as it was either generated by Lua or by 'msghandler'.
*/
int lua_report (lua_State *L, int status) {
  if (status != LUA_OK) {
    const char *msg = lua_tostring(L, -1);
    lua_l_message(g_progname, msg);
    lua_pop(L, 1);  /* remove message */
  }
  return status;
}
