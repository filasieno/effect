#pragma once

#include "lsp_basic.hpp"

/// \ingroup lsp_dynamic_types
/// \brief Dynamic value type enumeration
///
/// Defines all possible types that can be stored in dynamic values.
/// This enumeration is used as a discriminant in the tagged union.
///
enum lsp_dyn_kind {
    LSP_DYN_INVALID = 0,   ///< Undefined Value
    LSP_DYN_BOOLEAN_TRUE,  ///< Boolean value - True
    LSP_DYN_BOOLEAN_FALSE, ///< Boolean value - False
    LSP_DYN_INTEGER,       ///< Signed 64-bit integer
    LSP_DYN_UINTEGER,      ///< Unsigned 64-bit integer
    LSP_DYN_DECIMAL,       ///< Textual decimal representation (lossless)
    LSP_DYN_STRING,        ///< String value
    LSP_DYN_ARRAY,         ///< Array of dynamic values
    LSP_DYN_OBJECT         ///< Object with string keys and dynamic values
};


/// \defgroup lsp_dynamic_types Dynamic Types
/// \brief JSON-like dynamic value system for LSP protocol
/// 
/// Provides a type-safe way to represent JSON-like values without using floating-point numbers (doubles). 
/// Uses discriminated unions and tagged types for memory safety and performance.
struct lsp_dyn 
{
    enum lsp_dyn_kind kind; // lsp_dyn_kind
};

struct lsp_dyn_int : public lsp_dyn 
{
    signed long long  value;
};

struct lsp_dyn_uint : public lsp_dyn 
{
    unsigned long long value;
};

struct lsp_dyn_bool : public lsp_dyn 
{};

struct lsp_dyn_decimal : public lsp_dyn 
{
    double value; 
};

struct lsp_dyn_string : public lsp_dyn 
{
    struct lsp_string value;
};

struct lsp_dyn_array : public lsp_dyn 
{
    int             count;
    struct lsp_dyn* vec;
    int             vec_capacity;
};

struct lsp_dyn_attr {
    int                    height;
    struct lsp_string*     key;
    struct lsp_dyn*        value;
    struct lsp_dyn_object* left;
    struct lsp_dyn_object* right;
};

struct lsp_dyn_object : public lsp_dyn 
{
    int                  count;    
    struct lsp_dyn_attr* root;
};

static enum lsp_dyn_kind       lsp_dyn_get_kind(const struct lsp_dyn* dyn) noexcept;

static struct lsp_dyn_bool*    lsp_dyn_init_bool(void* mem, bool v) noexcept;
static bool                    lsp_dyn_bool_value(struct lsp_dyn_bool* expr) noexcept;

static struct lsp_dyn_int*     lsp_dyn_init_int(void* mem, signed long long v) noexcept;
static signed long long        lsp_dyn_int_value(struct lsp_dyn_int* expr) noexcept;

static struct lsp_dyn_uint*    lsp_dyn_init_uint(void* mem, unsigned long long v) noexcept;
static unsigned long long      lsp_dyn_uint_value(struct lsp_dyn_uint* expr) noexcept;

static struct lsp_dyn_decimal* lsp_dyn_init_decimal(void* mem, double v) noexcept;
static double                  lsp_dyn_decimal_value(struct lsp_dyn_decimal* expr) noexcept;

static struct lsp_dyn_string*  lsp_dyn_init_string(void* mem, const char* v) noexcept;
static const lsp_string*       lsp_dyn_string_value(struct lsp_dyn_string* expr) noexcept;

static struct lsp_dyn_array*   lsp_dyn_init_array(void* mem, int initial_capacity) noexcept;
static int                     lsp_dyn_array_count(struct lsp_dyn_array* expr) noexcept;
static struct lsp_dyn*         lsp_dyn_array_at(struct lsp_dyn_array* expr, int index) noexcept;
static struct lsp_dyn*         lsp_dyn_array_set_at(struct lsp_dyn_array* expr, int index, struct lsp_dyn* v) noexcept;
static void                    lsp_dyn_array_push(struct lsp_dyn_array* expr, struct lsp_dyn* v) noexcept;
static struct lsp_dyn*         lsp_dyn_array_pop(struct lsp_dyn_array* expr) noexcept;

static struct lsp_dyn_object*  lsp_dyn_init_object(void* mem) noexcept;
static int                     lsp_dyn_object_count(const struct lsp_dyn_object* expr) noexcept;
static struct lsp_dyn*         lsp_dyn_object_value_at(struct lsp_dyn_object* expr, const char* k) noexcept;
static void                    lsp_dyn_object_put_value(struct lsp_dyn_object* expr, const char* k, const lsp_dyn* v) noexcept;

