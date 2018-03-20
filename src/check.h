#ifndef CHECK_H
#define CHECK_H

#include "ast.h"
#include "basic.h"
#include "pool_allocator.h"

enum class Job_Type : u32
{
    typecheck,
};

struct Job
{
    Job_Type type;
    u32 stage;
    union {
        struct {
            Expr_AST *type;
            AST *ast;
        } typecheck;
    };
};

struct Context
{
    Pool_Allocator *ast_pool;
    Dynamic_Array<Job> *waiting_jobs;
    bool success;
};

enum class Status
{
    good,
    yield,
    bad,
};

Status types_match(Context *ctx, Expr_AST *t1, Expr_AST *t2, bool report = true);
Status reduce_type(Context *ctx, Expr_AST *type, Expr_AST **type_out);

// Make various job types
Job make_typecheck_job(Expr_AST *type, AST *ast, u32 stage = 0);

// Job execution functions return true if there was progress made

bool do_job(Context *ctx, Job job);
bool do_typecheck_job(Context *ctx, Job job);

bool typecheck_all(Pool_Allocator *ast_pool, Array<Decl_AST*> decls);


#endif // CHECK_H