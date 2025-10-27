#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

/* Forward Declarations */
typedef struct ExprNode ExprNode;
typedef struct ExprNodeList ExprNodeList;
typedef struct Field Field;
typedef struct FieldList FieldList;
typedef struct FuncCall FuncCall;
typedef struct FuncBody FuncBody;
typedef struct NameList NameList;
typedef struct StatNode StatNode;
typedef struct StatNodeList StatNodeList;
typedef struct VarNode VarNode;
typedef struct VarNodeList VarNodeList;

/* Grammar Structures */

struct ExprNode {
  enum {
    NilExpr,
    FalseExpr,
    TrueExpr,
    NumberExpr,
    StringExpr,
    VargExpr,
    FuncDecExpr,
    PrefixExpr,
    TableConstExpr,
    BinaryExpr,
    UnaryExpr
  } type;

  union {
    double number_expr;
    char *string_expr;
    VarNode *varg_expr;
    FuncBody *funcdec_expr;
    FieldList *tableconst_expr;

    struct {
      char op;
      ExprNode *left;
      ExprNode *right;
    } binary_expr;

    struct {
      char op;
      ExprNode *left;
    } unary_expr;

  } data;
};

struct ExprNodeList {
  ExprNode *curr_expr;
  ExprNodeList *next;
};

struct VarNode {
  enum { NameVar, IndexVar, MemberVar } type;
  union {
    char *name;
    struct {
      ExprNode *prefix;
      ExprNode *key;
      bool is_index;
    } complex_var;
  } data;
};

struct VarNodeList {
  VarNode *var;
  VarNodeList *next;
};

struct FuncBody {
  NameList *paralist;
  StatNodeList *block;
};

struct Field {
  enum { ArrayField, KeyValField, IndexField } type;
  char *name;
  ExprNode *Key;
  ExprNode *value;
};

struct FieldList {
  Field *field;
  FieldList *next;
};

struct FuncCall {
  enum { normal_call, member_call } type;
  char *member_name;
  ExprNodeList *arguments;
  ExprNode *prefix;
};

struct StatNode {
  enum {
    AssignmentStat,
    FuncCallStat,
    DoBlockStat,
    WhileBlockStat,
    RepeatBlockStat,
    IfBlockStat,
    IteratorForBlockStat,
    UnpackForBlockStat,
    FunctionDefStat,
    LocalFunctionDefStat,
    LocalVarListDefStat,
    ReturnStat,
    BreakStat,
    EmptyStat
  } type;

  union {

    struct {
      VarNodeList *varlist;
      ExprNodeList *exprlist;
    } assingment_stat;

    FuncCall *funccall_stat;

    struct {
      ExprNode *condition;
      StatNodeList *block;
    } dolike_block_stat;

    struct {
      ExprNode *condition;
      StatNodeList *block;
      StatNode *next_clause;
      bool is_else;
    } if_else_stat;

    struct {
      char *name;
      ExprNode *init;
      ExprNode *final;
      ExprNode *step;
      StatNodeList *block;
    } iteratorfor_stat;

    struct {
      NameList *namelist;
      ExprNodeList *explist;
      StatNodeList *block;
    } unpackfor_stat;

    struct {
      char *base_name;
      char *method_name;
      ExprNodeList *field_names;
      FuncBody *body;
    } functiondef_stat;

    struct {
      char *name;
      FuncBody *body;
    } localfunctiondef_stat;

    struct {
      NameList *namelist;
      ExprNodeList *explist;
    } localvarlist_stat;

    ExprNodeList *return_stat;

  } data;
};

struct StatNodeList {
  StatNode *stat;
  StatNodeList *next;
};

/* Helper functions for parsing each grammar rule */

StatNodeList *parse_chunk(); /* This is the main node that should be exposed */

// will add more helper function to parse all the grammar non-terminals

#endif // PARSER_H
