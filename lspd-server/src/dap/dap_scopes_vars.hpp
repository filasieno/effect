#pragma once

#include "dap_basic.hpp"

/// \file dap_scopes_vars.hpp
/// \brief DAP scopes and variables requests

// scopes
struct dap_scopes_arguments 
{
    int frame_id;
};
struct dap_scopes_response_body 
{ /* array<Scope> scopes; */
};
struct dap_scopes_request 
{
    struct dap_msg_hdr          hdr;
    struct dap_scopes_arguments arguments;
};
struct dap_scopes_response 
{
    struct dap_msg_hdr              hdr;
    struct dap_scopes_response_body body;
};
static struct dap_scopes_request *dap_init_scopes_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_scopes_response *dap_init_scopes_response(void *mem, AkU64 tag_id) noexcept;

// variables
struct dap_variables_arguments 
{
    int variables_reference;
    int filter;
    int start;
    int count;
    int format_hex;
};

struct dap_variables_response_body 
{ 
    /* array<Variable> variables; */
};

struct dap_variables_request
{
    struct dap_msg_hdr hdr;
    struct dap_variables_arguments arguments;
};


struct dap_variables_response
{
    struct dap_msg_hdr hdr;
    struct dap_variables_response_body body;
};

static struct dap_variables_request *dap_init_variables_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_variables_response *dap_init_variables_response(void *mem, AkU64 tag_id) noexcept;

// setVariable
struct dap_set_variable_arguments
{
    int         variables_reference;
    const char *name;
    const char *value;
    int         format_hex;
};

struct dap_set_variable_response_body
{
    const char *value;
    const char *type;
    int         variables_reference;
    int         named_variables;
    int         indexed_variables;
    const char *memory_reference;
    int         value_location_reference;
};

struct dap_set_variable_request
{
    struct dap_msg_hdr hdr;
    struct dap_set_variable_arguments arguments;
};

struct dap_set_variable_response
{
    struct dap_msg_hdr hdr;
    struct dap_set_variable_response_body body;
};

static struct dap_set_variable_request *dap_init_set_variable_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_set_variable_response *dap_init_set_variable_response(void *mem, AkU64 tag_id) noexcept;

// evaluate
struct dap_evaluate_arguments
{
    const char *expression;
    int         frame_id;
    int         line;
    int         column;
    int         has_source;
    int         context;
    int         format_hex;
};

struct dap_evaluate_response_body
{
    const char *result;
    const char *type;
    int         variables_reference;
    int         named_variables;
    int         indexed_variables;
    const char *memory_reference;
    int         value_location_reference;
};

struct dap_evaluate_request
{
    struct dap_msg_hdr hdr;
    struct dap_evaluate_arguments arguments;
};

struct dap_evaluate_response
{
    struct dap_msg_hdr hdr;
    struct dap_evaluate_response_body body;
};

static struct dap_evaluate_request *dap_init_evaluate_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_evaluate_response *dap_init_evaluate_response(void *mem, AkU64 tag_id) noexcept;

// setExpression
struct dap_set_expression_arguments
{
    const char *expression;
    const char *value;
    int frame_id;
    int format_hex;
};

struct dap_set_expression_response_body
{
    const char *value;
    const char *type;
    int variables_reference;
    int named_variables;
    int indexed_variables;
    const char *memory_reference;
};

struct dap_set_expression_request
{
    struct dap_msg_hdr hdr;
    struct dap_set_expression_arguments arguments;
};

struct dap_set_expression_response
{
    struct dap_msg_hdr hdr;
    struct dap_set_expression_response_body body;
};

static struct dap_set_expression_request *dap_init_set_expression_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_set_expression_response *dap_init_set_expression_response(void *mem, AkU64 tag_id) noexcept;

// exceptionInfo
struct dap_exception_info_arguments
{
    int thread_id;
};

struct dap_exception_info_response_body
{
    const char *exception_id;
    const char *description;
    int break_mode; /* ExceptionDetails* */
};

struct dap_exception_info_request
{
    struct dap_msg_hdr hdr;
    struct dap_exception_info_arguments arguments;
};

struct dap_exception_info_response
{
    struct dap_msg_hdr hdr;
    struct dap_exception_info_response_body body;
};

static struct dap_exception_info_request *dap_init_exception_info_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_exception_info_response *dap_init_exception_info_response(void *mem, AkU64 tag_id) noexcept;

// readMemory
struct dap_read_memory_arguments
{
    const char *memory_reference;
    int offset;
    int count;
};

struct dap_read_memory_response_body
{
    const char *address;
    int unreadable_bytes;
    const char *data;
};

struct dap_read_memory_request
{
    struct dap_msg_hdr hdr;
    struct dap_read_memory_arguments arguments;
};

struct dap_read_memory_response
{
    struct dap_msg_hdr hdr;
    struct dap_read_memory_response_body body;
};

static struct dap_read_memory_request *dap_init_read_memory_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_read_memory_response *dap_init_read_memory_response(void *mem, AkU64 tag_id) noexcept;

// writeMemory
struct dap_write_memory_arguments
{
    const char *memory_reference;
    int offset;
    int allow_partial;
    const char *data;
};

struct dap_write_memory_response_body
{
    int offset;
    int bytes_written;
};

struct dap_write_memory_request
{
    struct dap_msg_hdr hdr;
    struct dap_write_memory_arguments arguments;
};

struct dap_write_memory_response
{
    struct dap_msg_hdr hdr;
    struct dap_write_memory_response_body body;
};

static struct dap_write_memory_request *dap_init_write_memory_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_write_memory_response *dap_init_write_memory_response(void *mem, AkU64 tag_id) noexcept;

// disassemble
struct dap_disassemble_arguments
{
    const char *memory_reference;
    int offset;
    int instruction_offset;
    int instruction_count;
    int resolve_symbols;
};

struct dap_disassemble_response_body
{ /* array<DisassembledInstruction> instructions; */
};

struct dap_disassemble_request
{
    struct dap_msg_hdr hdr;
    struct dap_disassemble_arguments arguments;
};

struct dap_disassemble_response
{
    struct dap_msg_hdr hdr;
    struct dap_disassemble_response_body body;
};

static struct dap_disassemble_request *dap_init_disassemble_request(void *mem, AkU64 tag_id) noexcept;
static struct dap_disassemble_response *dap_init_disassemble_response(void *mem, AkU64 tag_id) noexcept;