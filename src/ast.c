/*
 * Copyright (c) 2020/08 gravieb <truongptk30a3@gmail.com>
 * This file is released under the GPLv3
 */

#include <string.h>
#include "log.h"
#include "ast.h"
#include "lex.h"
#include "gen.h"
#include "parse_internal.h"

static int32_t tok_2_ast (Token token);

int32_t tok_2_ast (Token token)
{
    int ast = -1;
    switch (token.tok) {
    case TokenPlus:
        ast = AstPlus;
        break;
    case TokenMinus:
        ast = AstMinus;
        break;
    case TokenMul:
        ast = AstMul;
        break;
    case TokenDiv:
        ast = AstDiv;
        break;
    case TokenNumber:
        ast = AstNumber;
        break;
    case TokenPrint:
        ast = AstPrint;
        break;
    case TokenIntType:
        ast = AstIntType;
        break;
    case TokenIdentifier:
        ast = token.left_value ? AstLeftVar : AstRightVar;
        break;
    case TokenAssign:
        ast = AstAssign;
        break;
    default:
        mlog(CLGT,"Can not create AstNode with token: %s\n", tok2str(token.tok));
    }

    return ast;

}

AstNode* ast_create_node(
    Token token,
    AstNode* left,
    AstNode* right)
{
    AstNode* node = (AstNode*) malloc(sizeof(AstNode));
    int32_t ast_type = tok_2_ast(token);
    switch (ast_type)
    {
    case AstNumber:
        node->value = token.value;
        break;
    case AstLeftVar:
    case AstRightVar:
        node->id_str = (char*)malloc(strlen(token.id_str)+1);
        memcpy(node->id_str, token.id_str, strlen(token.id_str));
        node->id_str[strlen(token.id_str)] = '\0';
        break;
    default:
        mlog(TRACE, "AstNode %d\n", ast_type);
    }

    node->type = ast_type;
    node->left = left;
    node->right = right;

    return node;
}

AstNode* ast_create_leaf(Token token)
{
    return ast_create_node(token, NULL, NULL);
}

AstNode* ast_create_unary(Token token, AstNode* left)
{
    return ast_create_node(token, left, NULL);
}

void* ast_compile(void* gen_prm, AstNode* node)
{
    char *left = NULL, *right = NULL;
    if (NULL != node->left) {
        left = (char*)ast_compile(gen_prm, node->left);
    }
    if (NULL != node->right) {
        right = (char*)ast_compile(gen_prm, node->right);
    }

    switch (node->type) {
    case AstNumber:
        return GenLoad(gen_prm,node->value);
    case AstPrint:
        return GenPrint(gen_prm, left);
    case AstIntType:
        return node;
    case AstLeftVar:
        if (NULL != left && AstIntType == ((AstNode*)left)->type) {
            // declare variable
            return GenVar(gen_prm, node->id_str);
        }
        return node->id_str;
    case AstRightVar:
        return GenLoadVar(gen_prm, node->id_str);
    case AstAssign:
        return GenStore(gen_prm, left, right);
    case AstPlus:
        return GenPlus(gen_prm, left, right);
    case AstMinus:
        return GenMinus(gen_prm, left, right);
    case AstMul:
        return GenMul(gen_prm, left, right);
    case AstDiv:
        return GenDiv(gen_prm, left, right);
    default:
        mlog(CLGT,"Not yet to support ast type %d\n",node->type);
    }
    return NULL;
}

void ast_gen(ParseParameter* parse_prm, AstNode* node)
{
    if (parse_prm->is_interpret) {
        // todo:
        // ast_interpret(parse_prm, node);
    } else {
        char* r = ast_compile(parse_prm->gen_prm, node);
    }
}
