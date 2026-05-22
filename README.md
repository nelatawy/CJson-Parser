# JSON Parser - Recursive Descent Parser

A lightweight C-based JSON parser using **recursive descent parsing** to build an Abstract Syntax Tree (AST) representation of JSON data.

---

## Data Structures

### `node_type` (Enum)
Represents the type of JSON value:
```c
typedef enum{
    STRING,   // "hello"
    NUMBER,   // 42
    BOOL,     // true/false
    NONE,     // null
    ARRAY,    // [...]
    OBJECT    // {...}
} node_type;
```

### `node_val` (Union)
Stores the value based on node type:
```c
typedef union {
    int num_val;           // for NUMBER, BOOL
    char* str_val;         // for STRING
    node* first_child;     // for ARRAY, OBJECT (pointer to child linked list)
} node_val;
```

### `node` (Struct)
Represents a single JSON element in a doubly-linked list:
```c
typedef struct node {
    node_type type;        // what kind of value is this?
    node_val val;          // the actual value
    char* name;            // key name (for OBJECT elements only)
    node* next;            // next sibling in list
    node* prev;            // previous sibling in list
} node;
```

**Why doubly-linked list?** Each OBJECT has multiple key-value pairs, and ARRAY has multiple elements. These are stored as chains of `node` siblings.

---

## Core Parsing Methods

### Entry Point
- **`parse_json(char* str)`** → `node*`
  - Parses top-level JSON (expects `{...}`)
  - Returns root node or NULL on error

### Recursive Descent Flow

1. **`parse_object(char** str)`** → `node*`
   - Parses `{key: value, key: value, ...}`
   - Calls `parse_key_val()` for each key-value pair
   - Builds linked list of children
   - Returns OBJECT node

2. **`parse_array(char** str)`** → `node*`
   - Parses `[value, value, ...]`
   - Calls `parse_val()` for each element
   - Builds linked list of children
   - Returns ARRAY node

3. **`parse_key_val(char** str)`** → `node*`
   - Parses a single `"key": value` pair
   - Extracts key name and stores in `node->name`
   - Calls `parse_val()` to parse the value
   - Returns node with value

4. **`parse_val(char** str)`** → `node*`
   - Dispatcher: peeks at next character and decides what to parse
   - Calls: `parse_object()`, `parse_array()`, `parse_string()`, `parse_number()`, or handles `true`/`false`/`null`
   - **This is where recursion happens!**

5. **`parse_string(char** str)`** → `node*`
   - Parses quoted string, allocates memory for string value

6. **`parse_number(char** str)`** → `node*`
   - Parses integer numbers, converts to `int`

### Utility Methods
- **`skip_whitespaces(char** str)`** - Advances pointer past spaces/tabs/newlines
- **`free_tree(node* root)`** - Recursively frees all allocated memory

---

## Recursive Descent Parsing Explained

This parser uses **recursive descent** because:
- **Recursive**: Functions call each other recursively
- **Descent**: Moves "down" the grammar hierarchy

### Example: Parsing `{"name": "John", "hobbies": ["reading", "coding"]}`

```
parse_json()
  └─ parse_object()
      ├─ parse_key_val() for "name": "John"
      │   └─ parse_val() detects "
      │       └─ parse_string() → returns STRING node
      │
      └─ parse_key_val() for "hobbies": [...]
          └─ parse_val() detects [
              └─ parse_array()
                  ├─ parse_val() for "reading"
                  │   └─ parse_string()
                  └─ parse_val() for "coding"
                      └─ parse_string()
```

Each level of nesting triggers deeper recursion. Arrays inside objects call back to `parse_val()` → `parse_array()` → `parse_val()` again.

---

## How to Use

### Basic Example
```c
#include "json_parser.h"
#include <stdio.h>

int main() {
    // Simple JSON string
    char json[] = "{ \"name\" : \"alex\", \"friends\" : [\"joseph\"]}";
    
    // Parse it
    node* root = parse_json(json);
    
    if (!root) {
        printf("Invalid JSON\n");
        return 1;
    }
    
    // Navigate the tree
    // root->type == OBJECT
    // root->val.first_child points to first key-value pair ("name": "alex")
    
    node* name_node = root->val.first_child;           // First child (name)
    printf("Key: %s\n", name_node->name);              // Output: "name"
    printf("Value: %s\n", name_node->val.str_val);     // Output: "alex"
    
    node* friends_node = name_node->next;              // Second child (friends)
    printf("Key: %s\n", friends_node->name);           // Output: "friends"
    // friends_node->type == ARRAY
    
    node* first_friend = friends_node->val.first_child; // First array element
    printf("Friend: %s\n", first_friend->val.str_val);  // Output: "joseph"
    
    // Clean up
    free_tree(root);
    return 0;
}
```

### Traversal Tips
- **Access first element**: `node->val.first_child` (for OBJECT/ARRAY)
- **Move to siblings**: Use `node->next` to iterate through key-value pairs or array elements
- **Get object key**: `node->name` (only set for OBJECT values)
- **Get value**: `node->val.str_val` (STRING), `node->val.num_val` (NUMBER/BOOL)
- **Check type**: `node->type` to determine which union member to access

### Using the `get()` Method

The `get()` function provides convenient access to child nodes without manual traversal:

```c
node* get(node* src, char* name);
```

- **For OBJECT access**: Pass the key name
  ```c
  node* name = get(root, "name");           // Access by key
  printf("%s\n", name->val.str_val);        // Output: "alex"
  ```

- **For ARRAY access**: Pass the index as a string
  ```c
  node* first = get(array_node, "0");       // Access first element
  node* second = get(array_node, "1");      // Access second element
  printf("%s\n", first->val.str_val);
  ```

- **Return value**: Returns the node if found, or NULL if not found or if src is not an OBJECT/ARRAY

### Cleanup
Always call `free_tree(root)` when done to avoid memory leaks. The function recursively frees all children.

---

## Summary

| Component | Purpose |
|-----------|---------|
| **node_type** | Identifies what kind of JSON value |
| **node_val** | Holds the actual value (polymorphic via union) |
| **node** | Tree node with sibling pointers for multiple values |
| **parse_json()** | Entry point, kicks off parsing |
| **parse_object/array()** | Handles composite types (recurse downward) |
| **parse_val()** | Dispatcher; peeks and routes to correct parser |
| **parse_string/number()** | Leaf parsers for primitive values |

The recursive descent pattern elegantly handles nested structures by having higher-level parsers (`parse_object`, `parse_array`) call lower-level ones (`parse_val`, `parse_string`) which can call back up, creating a mutual recursion that mirrors JSON's recursive structure.
