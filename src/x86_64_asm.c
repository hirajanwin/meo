/*
 * Copyright (c) 2020/07 gravieb <truongptk30a3@gmail.com>
 * This file is released under the GPLv3
 */

#include <string.h>
#include "log.h"
#include "gen.h"
#include "gen_internal.h"
#include "error_code.h"

static char* x86_64_load(int32_t value, FILE* out_file);
static char* x86_64_free(char* r, FILE* out_file);
static char* x86_64_out(char* value, FILE* out_file);
static char* x86_64_print(char* r, FILE* out_file);
static char* x86_64_var(char* var, FILE* out_file);
static char* x86_64_add(char* r1, char* r2, FILE* out_file);
static char* x86_64_sub(char* r1, char* r2, FILE* out_file);
static char* x86_64_mul(char* r1, char* r2, FILE* out_file);
static char* x86_64_div(char* r1, char* r2, FILE* out_file);
static char* x86_64_lt(char* r1, char* r2, FILE* out_file);
static char* x86_64_le(char* r1, char* r2, FILE* out_file);
static char* x86_64_gt(char* r1, char* r2, FILE* out_file);
static char* x86_64_ge(char* r1, char* r2, FILE* out_file);
static char* x86_64_eq(char* r1, char* r2, FILE* out_file);
static char* x86_64_ne(char* r1, char* r2, FILE* out_file);
static char* x86_64_lt_j(char* r1, char* r2, char* label, FILE* out_file);
static char* x86_64_le_j(char* r1, char* r2, char* label, FILE* out_file);
static char* x86_64_gt_j(char* r1, char* r2, char* label, FILE* out_file);
static char* x86_64_ge_j(char* r1, char* r2, char* label, FILE* out_file);
static char* x86_64_eq_j(char* r1, char* r2, char* label, FILE* out_file);
static char* x86_64_ne_j(char* r1, char* r2, char* label, FILE* out_file);
static char* x86_64_jump(char* label, FILE* out_file);
static char* x86_64_zero_j(char* r, char* label, FILE* out_file);
static char* x86_64_label(char* label, FILE* out_file);
static char* x86_64_func(char* name, FILE* out_file);

static char* x86_64_store(char* r, char* var, FILE* out_file);
static char* x86_64_load_var(char* var, FILE* out_file);
static char* x86_64_return(char* r, FILE* out_file);

typedef struct RegMap {
    char* reg64;
    char* reg8;
} RegMap;

static char* reg[] = {
    "%r8",
    "%r9",
    "%r10",
    "%r11",
    "%r12",
    "%r13",
    "%r14",
    "%r15"};

static RegMap reg_map[] = {
    {"%r8",  "%r8b"},
    {"%r9",  "%r9b"},
    {"%r10", "%r10b"},
    {"%r11", "%r11b"},
    {"%r12", "%r12b"},
    {"%r13", "%r13b"},
    {"%r14", "%r14b"},
    {"%r15", "%r15b"}
};

static int cur_reg = 0;

int32_t GenLoadX86_64(GenFuncTable *func)
{
    func->f_load   = &x86_64_load;
    func->f_free   = &x86_64_free;
    func->f_out    = &x86_64_out;
    func->f_var    = &x86_64_var;
    func->f_print  = &x86_64_print;
    func->f_add    = &x86_64_add;
    func->f_sub    = &x86_64_sub;
    func->f_mul    = &x86_64_mul;
    func->f_div    = &x86_64_div;
    func->f_lt     = &x86_64_lt;
    func->f_le     = &x86_64_le;
    func->f_gt     = &x86_64_gt;
    func->f_ge     = &x86_64_ge;
    func->f_eq     = &x86_64_eq;
    func->f_ne     = &x86_64_ne;
    func->f_lt_j   = &x86_64_lt_j;
    func->f_le_j   = &x86_64_le_j;
    func->f_gt_j   = &x86_64_gt_j;
    func->f_ge_j   = &x86_64_ge_j;
    func->f_eq_j   = &x86_64_eq_j;
    func->f_ne_j   = &x86_64_ne_j;
    func->f_jump   = &x86_64_jump;
    func->f_zero_j = &x86_64_zero_j;
    func->f_label  = &x86_64_label;
    func->f_func   = &x86_64_func;
    func->f_store  = &x86_64_store;
    func->f_load_var = &x86_64_load_var;
    func->f_return = &x86_64_return;
    cur_reg = 0;
    return Success;
}

char* reg_alloc()
{
    if (cur_reg >= sizeof(reg)/sizeof(*reg)) {
        MLOG(CLGT,"Not availabel register\n");
        return NULL;
    }
    return reg[cur_reg++];
}

void reg_free(char* r)
{
    if (0 == cur_reg) {
        MLOG(CLGT,"Free register invalid\n");
        exit(1);
    }
    reg[--cur_reg] = r;
}

char* reg64_to_8(char* r)
{
    for (int i = 0; i < sizeof(reg_map)/sizeof(reg_map[0]); i++) {
        if (!strncmp(reg_map[i].reg64 , r, strlen(r))) {
            return reg_map[i].reg8;
        }
    }
    return NULL;
}

char* x86_64_load(int32_t value, FILE* out_file)
{
    char* r = reg_alloc();
    fprintf(out_file,"\tmovq $%d, %s\n",value,r);
    return r;
}

char* x86_64_free(char* r, FILE* out_file)
{
    reg_free(r);
    return r;
}

char* x86_64_out(char* r, FILE* out_file)
{
    fprintf(out_file,"\tmovq %s, %s\n",r,"%rax");
    fprintf(out_file,"\tret\n");
    reg_free(r);
    return r;
}

char* x86_64_print(char* r, FILE* out_file)
{
    fprintf(out_file,"\tmovq %s, %%rsi\n",r);
    fprintf(out_file,"\tmovq $.LC0, %%rdi\n");
    fprintf(out_file,"\tmovq $0, %%rax\n");
    fprintf(out_file,"\tcall printf\n");
    reg_free(r);
    return r;
}

char* x86_64_var(char* var, FILE* out_file)
{
    fprintf(out_file,"\t.comm\t%s,8,8\n",var);
    return var;
}

char* x86_64_add(char* r1, char* r2, FILE* out_file)
{
    fprintf(out_file,"\taddq %s, %s\n",r1,r2);
    reg_free(r1);
    return r2;
}

char* x86_64_sub(char* r1, char* r2, FILE* out_file)
{
    fprintf(out_file,"\tsubq %s, %s\n",r2,r1);
    reg_free(r2);
    return r1;
}

char* x86_64_mul(char* r1, char* r2, FILE* out_file)
{
    fprintf(out_file,"\timulq %s, %s\n",r1,r2);
    reg_free(r1);
    return r2;
}

char* x86_64_div(char* r1, char* r2, FILE* out_file)
{
    fprintf(out_file,"\tmovq %s, %s\n",r1,"%rax");
    fprintf(out_file, "\tcqo\n");
    fprintf(out_file,"\tidivq %s\n",r2);
    fprintf(out_file,"\tmovq %s, %s\n","%rax",r2);
    reg_free(r1);
    return r2;
}

char* x86_64_lt(char* r1, char* r2, FILE* out_file)
{
    fprintf(out_file, "\tcmpq %s, %s\n", r2, r1);
    fprintf(out_file, "\tsetl %s\n", reg64_to_8(r2));
    fprintf(out_file, "\tandq $255,%s\n", r2);
    reg_free(r1);
    return r2;
}

char* x86_64_le(char* r1, char* r2, FILE* out_file)
{
    fprintf(out_file, "\tcmpq %s, %s\n", r2, r1);
    fprintf(out_file, "\tsetle %s\n", reg64_to_8(r2));
    fprintf(out_file, "\tandq $255,%s\n", r2);
    reg_free(r1);
    return r2;
}

char* x86_64_gt(char* r1, char* r2, FILE* out_file)
{
    fprintf(out_file, "\tcmpq %s, %s\n", r2, r1);
    fprintf(out_file, "\tsetg %s\n", reg64_to_8(r2));
    fprintf(out_file, "\tandq $255,%s\n", r2);
    reg_free(r1);
    return r2;
}

char* x86_64_ge(char* r1, char* r2, FILE* out_file)
{
    fprintf(out_file, "\tcmpq %s, %s\n", r2, r1);
    fprintf(out_file, "\tsetge %s\n", reg64_to_8(r2));
    fprintf(out_file, "\tandq $255,%s\n", r2);
    reg_free(r1);
    return r2;
}

char* x86_64_eq(char* r1, char* r2, FILE* out_file)
{
    fprintf(out_file, "\tcmpq %s, %s\n", r2, r1);
    fprintf(out_file, "\tsete %s\n", reg64_to_8(r2));
    fprintf(out_file, "\tandq $255,%s\n", r2);
    reg_free(r1);
    return r2;
}

char* x86_64_ne(char* r1, char* r2, FILE* out_file)
{
    fprintf(out_file, "\tcmpq %s, %s\n", r2, r1);
    fprintf(out_file, "\tsetne %s\n", reg64_to_8(r2));
    fprintf(out_file, "\tandq $255,%s\n", r2);
    reg_free(r1);
    return r2;
}

char* x86_64_lt_j(char* r1, char* r2, char* label, FILE* out_file)
{
    fprintf(out_file, "\tcmpq %s, %s\n", r2, r1);
    fprintf(out_file, "\tjl %s\n", label);
    reg_free(r1);
    reg_free(r2);
    return label;
}

char* x86_64_le_j(char* r1, char* r2, char* label, FILE* out_file)
{
    fprintf(out_file, "\tcmpq %s, %s\n", r2, r1);
    fprintf(out_file, "\tjle %s\n", label);
    reg_free(r1);
    reg_free(r2);
    return label;
}

char* x86_64_gt_j(char* r1, char* r2, char* label, FILE* out_file)
{
    fprintf(out_file, "\tcmpq %s, %s\n", r2, r1);
    fprintf(out_file, "\tjg %s\n", label);
    reg_free(r1);
    reg_free(r2);
    return label;
}

char* x86_64_ge_j(char* r1, char* r2, char* label, FILE* out_file)
{
    fprintf(out_file, "\tcmpq %s, %s\n", r2, r1);
    fprintf(out_file, "\tjge %s\n", label);
    reg_free(r1);
    reg_free(r2);
    return label;
}

char* x86_64_eq_j(char* r1, char* r2, char* label, FILE* out_file)
{
    fprintf(out_file, "\tcmpq %s, %s\n", r2, r1);
    fprintf(out_file, "\tje %s\n", label);
    reg_free(r1);
    reg_free(r2);
    return label;
}

char* x86_64_ne_j(char* r1, char* r2, char* label, FILE* out_file)
{
    fprintf(out_file, "\tcmpq %s, %s\n", r2, r1);
    fprintf(out_file, "\tjne %s\n", label);
    reg_free(r1);
    reg_free(r2);
    return label;
}

char* x86_64_jump(char* label, FILE* out_file)
{
    fprintf(out_file, "\tjmp %s\n", label);
    return label;
}

char* x86_64_zero_j(char* r, char* label, FILE* out_file)
{
    fprintf(out_file, "\tcmpq $0, %s\n", r);
    fprintf(out_file, "\tje %s\n", label);
    reg_free(r);
    return label;
}

char* x86_64_label(char* label, FILE* out_file)
{
    fprintf(out_file, "\t%s:\n", label);
    return label;
}

char* x86_64_func(char* name, FILE* out_file)
{
    fprintf(out_file, "\t.text\n");
    fprintf(out_file, "\t.globl\t%s\n", name);
    fprintf(out_file, "\t.type\t%s, @function\n", name);
    fprintf(out_file, "%s:\n", name);
    fprintf(out_file, "\tpushq\t%%rbp\n");
    fprintf(out_file, "\tmovq\t%%rsp, %%rbp\n");
    return name;
}

char* x86_64_store(char* var, char* r, FILE* out_file)
{
    fprintf(out_file, "\tmovq %s, %s(\%%rip)\n", r, var);
    reg_free(r);
    return var;
}

char* x86_64_load_var(char* var, FILE* out_file)
{
    char* r = reg_alloc();
    fprintf(out_file, "\tmovq %s(\%%rip), %s\n", var, r);
    return r;
}

static char* x86_64_return(char* r, FILE* out_file)
{
    fprintf(out_file, "\tmovq %s, %%rax\n", r);
    fprintf(out_file, "\tpopq %%rbp\n");
    fprintf(out_file, "\tret\n");
    return r;
}
