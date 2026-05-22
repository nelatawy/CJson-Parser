#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <malloc.h>
#include "json_parser.h"

/**
 * skip_whitespaces - Advances the string pointer past whitespace characters
 * @param str: pointer to the current position in the JSON string
 * 
 * Skips spaces, tabs, and newlines to simplify token parsing.
 */
void skip_whitespaces(char** str){
    while((**str) && (**str == ' ' || **str == '\t' || **str == '\n')){
        *str += 1;
        
    }
}

/**
 * parse_val - Dispatcher that parses any JSON value type
 * @param str pointer to the current position in the JSON string
 * 
 * Peeks at the next character and delegates to the appropriate parser:
 * '{' -> parse_object(), '[' -> parse_array(), '"' -> parse_string(),
 * digits/'-' -> parse_number(), 'true'/'false'/'null' -> handle directly
 * 
 * @return node representing the parsed value, or NULL on error
 */
node* parse_val(char** str){
    skip_whitespaces(str);
    node* n = NULL;
    if (**str =='{'){   
        n = parse_object(str);
    } 
    else if (**str == '['){
        n = parse_array(str);
    }
    else if(strcmp(*str, "null") == 0){
        *str += 4; // move forward
        n = (node*)malloc(sizeof(node));
        n->type = NONE;
    }
    else if(**str == '\"'){

        n = parse_string(str);
    }
    else if(**str == '-' || isdigit(**str)){
        n = parse_number(str);
    } 
    else if(strcmp(*str, "true") == 0){
        *str += 4; // move forward
        n = (node*)malloc(sizeof(node));
        n->type = BOOL;
        n->val.num_val = 1;
    }
    else if(strcmp(*str, "false") == 0){
        *str += 5; // move forward
        n = (node*)malloc(sizeof(node));
        n->type = BOOL;
        n->val.num_val = 0;
    }
    else if(!n){ // error
        return NULL;
    }
    return n;
}

/**
 * parse_key_val - Parses a single key-value pair in a JSON object
 * @param str: pointer to the current position in the JSON string
 * 
 * Expects format: "key" : value
 * Extracts the key name and stores it in the returned node.
 * Calls parse_val() to parse the value after the colon.
 * 
 * @return node with the value and name set, or NULL on error
 */
node* parse_key_val(char** str){
    skip_whitespaces(str);
    char* itr = *str;
    if (**str != '\"')
        return NULL;

    // printf("parsing key val\n");
    *str += 1;
    itr = *str;
    while(*itr != '\"')
        itr++;
    // parsing the name
    size_t key_len = itr - *str + 1;
    
    char* key = malloc(key_len + 1);
   
    
    *itr = '\0'; //trick strcpy
    strcpy(key, *str);

    *itr = '\"'; // return to original
    
    *str = itr + 1; //move the pointer past the name

    skip_whitespaces(str);
    if(**str != ':'){
        free(key);
        return NULL;
    }
    *str += 1;

    skip_whitespaces(str);
    
    //start processing the value
    node* n = parse_val(str);
    if (!n)
    {
        free(key);
        return NULL;
    }
    
    n->name = key;
    return n;
}

/**
 * parse_object - Parses a JSON object {...}
 * @param str pointer to the current position in the JSON string
 * 
 * Expects format: { "key1": value1, "key2": value2, ... }
 * Creates an OBJECT node and builds a linked list of key-value pairs as children.
 * Returns an empty OBJECT node if the object is empty {}.
 * 
 * @return OBJECT node with children linked list, or NULL on error
 */
node* parse_object(char** str){
    skip_whitespaces(str);
    if(**str != '{')//malformed
        return NULL;
    *str += 1;
    node* n = (node*)malloc(sizeof(node));
    n->type = OBJECT;

    skip_whitespaces(str);
    if(**str == '}'){//empty
        *str += 1;
        return n;
    }
    node* first_child = NULL;
    node* last_itr = NULL;

    while(**str){
        skip_whitespaces(str);
  
        node* child = parse_key_val(str);
        if (!child){
            free_tree(n);
            return NULL;
        }
        
        // link to the chain
        if(first_child == NULL){
            first_child = child;
            last_itr = child;
            n->val.first_child = first_child;
        } 
        else{
            last_itr->next = child;
            child->prev = last_itr;
            last_itr = child;
        }

        skip_whitespaces(str);
        // printf("%c\n", **str);
        if(**str == '}'){
            break; // exit the loop
        } else if(**str == ','){
            *str += 1; // go to next child
        } else { //error
            free_tree(n);
            return NULL;
        }
    }
    
    if(!(**str)){ //unbalanced
        free_tree(n);
        return NULL;
    }
    *str += 1; // skip the last brace
    return n;
}

/**
 * parse_array - Parses a JSON array [...]
 * @param str pointer to the current position in the JSON string
 * 
 * Expects format: [value1, value2, ...]
 * Creates an ARRAY node and builds a linked list of values as children.
 * Each element is given a numeric name (0, 1, 2, ...) for uniform access via get().
 * Returns an empty ARRAY node if the array is empty [].
 * 
 * @return ARRAY node with children linked list, or NULL on error
 */
node* parse_array(char** str){
    skip_whitespaces(str);
    if(**str != '[')//malformed
        return NULL;
    *str += 1;
    node* n = (node*)malloc(sizeof(node));
    n->type = ARRAY;

    skip_whitespaces(str);
    if(**str == ']'){//empty
        *str += 1;
        return n;
    }
        

    char* itr = *str;
    node* first_child = NULL;
    node* last_itr = NULL;

    int added_cnt = 0;
    while(**str){
        skip_whitespaces(str);
  
        node* child = parse_val(str);
        if (!child){
            free_tree(n);
            return NULL;
        }
        char* name = (char*)malloc(20);
        snprintf(name, sizeof(name), "%d", added_cnt++);
        child->name = name; 
        //to allow for get() to work as array access
        // link to the chain
        if(first_child == NULL){
            first_child = child;
            last_itr = child;
            n->val.first_child = first_child;
        } 
        else{
            last_itr->next = child;
            child->prev = last_itr;
            last_itr = child;
        }

        skip_whitespaces(str);
        if(**str == ']') {
            break; // exit the loop
        } else if(**str == ','){
            *str += 1; // go to next child
        } else { //error
            free_tree(n);
            return NULL;
        }
    }
    
    if(!(**str)){ //unbalanced
        free_tree(n);
        return NULL;
    }
    *str += 1; // skip the last bracket
    return n;
}

/**
 * parse_string - Parses a quoted JSON string
 * @param str pointer to the current position in the JSON string
 * 
 * Expects format: "string content"
 * Allocates memory for the string and stores it in the node's val.str_val.
 * 
 * @return STRING node with allocated string value, or NULL on error
 */
node* parse_string(char** str){
    skip_whitespaces(str);
    if (**str != '\"')
        return NULL;
    *str += 1;
    char* itr = *str;

    while (*itr && *itr != '\"'){
        itr++;
    }
    if (!(*itr)){  //unbalanced
        return NULL;
    }

    size_t len = itr - *str + 1;
    char* val = (char*)malloc(len + 1);
    *itr = '\0';
    strcpy(val, *str);

    *itr = '\"';

    *str = itr + 1; //move the pointer past the parsed

    node* n = (node*) malloc(sizeof(node));
    n->type = STRING;
    n->val.str_val = val;
    return n;
}

/**
 * parse_number - Parses a JSON number
 * @param str pointer to the current position in the JSON string
 * 
 * Parses integer values (including negative numbers).
 * Converts the string to an int using strtol().
 * 
 * @return NUMBER node with integer value, or NULL on error
 */
node* parse_number(char** str){
    skip_whitespaces(str);
    char* itr = *str;

    while (*itr && isdigit(*itr)){
        itr++;
    }
    if (!(*itr)){  //unbalanced
        return NULL;
    }

    size_t len = itr - *str + 1;
    char old_val = *itr;

    *itr = '\0';

    int val = (int)strtol(*str, NULL, 10);
    
    *itr = old_val;

    *str = itr; //move the pointer past the parsed

    node* n = (node*) malloc(sizeof(node));
    n->type = NUMBER;
    n->val.num_val = val;
    return n;
}

/**
 * free_tree - Recursively frees all allocated memory in the AST
 * @param root the root node of the tree to free
 * 
 * Traverses the linked list of children and recursively frees each subtree,
 * then frees the names and string values before freeing the nodes themselves.
 */
void free_tree(node* root){
    node* child_itr = root->val.first_child;
    while (child_itr)
    {   
        node* temp = child_itr->next;
        free_tree(child_itr);
        child_itr = temp;
    }
    
}

/**
 * parse_json - Entry point for parsing a JSON string
 * @param str: the JSON string to parse
 * 
 * Expects a JSON object at the top level {...}.
 * Creates a pointer-to-pointer to handle string advancement transparently.
 * Skips leading whitespace and validates the opening brace.
 * 
 * @return root OBJECT node representing the entire JSON structure, or NULL on error
 */
node* parse_json(char* str) { 
    char**itr = &str; 
    // to make the signature easier to cope with
    // and to make the changes done in pointer traversal transparent to the passed pointer
    skip_whitespaces(itr);
    if (**itr != '{') //malformed
        return NULL;
    node* res = parse_object(itr);
    return res;
}


/**
 * get - Retrieves a child node by name from an OBJECT or ARRAY
 * @param src the parent OBJECT or ARRAY node
 * @param name the key name (for OBJECT) or index as string (for ARRAY, e.g., "0", "1")
 * 
 * Searches the linked list of children for a node with matching name.
 * For ARRAY access, pass the index as a string (e.g., "2" for third element).
 * For OBJECT access, pass the key name (e.g., "name", "age").
 * 
 * @return the child node if found, NULL otherwise
 */
node* get(node* src, char* name){
    if(!src || (src->type != ARRAY && src->type != OBJECT) )
        return NULL;
    node* itr = src->val.first_child;
    while(itr){
        if (strcmp(itr->name, name) == 0)
            return itr;
        itr = itr->next;
    }
    return NULL;
}


// ---Testing----
int main(){
    char json[] = "{ \"name\" : \"alex\", \"friends\" : 5}";
    node* res = parse_json(json);
    if (!res)
    {
        printf("invalid json\n");
    } else {
        printf("%s\n", get(res, "name")->name);
    }
    
}