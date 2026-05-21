#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <malloc.h>
#include "json_parser.h"

void skip_whitespaces(char** str){
    // // printf("moving pointer\n");
    while((**str) && (**str == ' ' || **str == '\t' || **str == '\n')){
        // printf("%c moving\n", **str);
        *str += 1;
        
    }
    // printf("done skipping\n");
}

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
        // printf("parsing str\n");
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
        // printf("err parsing");
        return NULL;
    }
    return n;
}

/* Helper used in parse_object and parse_array*/
node* parse_key_val(char** str){
    skip_whitespaces(str);
    char* itr = *str;
    // printf("%c\n", **str);
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
    // printf("%s\n", key);
    *itr = '\"'; // return to original
    
    *str = itr + 1; //move the pointer past the name

    skip_whitespaces(str);
    if(**str != ':'){
        free(key);
        return NULL;
    }
    *str += 1;

    skip_whitespaces(str);
    // printf("%s\n", *str);
    
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

node* parse_object(char** str){
    // // printf("%d\n", **str == ' ');
    skip_whitespaces(str);
    if(**str != '{')//malformed
        return NULL;
    *str += 1;
    node* n = (node*)malloc(sizeof(node));
    n->type = OBJECT;

    skip_whitespaces(str);
    if(**str == '}')//empty
        return n;
    // printf("parsing object\n");
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

node* parse_array(char** str){
    skip_whitespaces(str);
    if(**str != '[')//malformed
        return NULL;
    *str += 1;
    node* n = (node*)malloc(sizeof(node));
    n->type = ARRAY;

    skip_whitespaces(str);
    if(**str == ']')//empty
        return n;

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
        if(**str == ']'){
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
    // printf("%s\n", val);
    *itr = '\"';

    *str = itr + 1; //move the pointer past the parsed

    node* n = (node*) malloc(sizeof(node));
    n->type = STRING;
    n->val.str_val = val;
    return n;
}

node* parse_number(char** str){
    skip_whitespaces(str);
    char* itr = *str;

    while (*itr && isdigit(*itr)){
        itr++;
    }
    if (!*itr){  //unbalanced
        return NULL;
    }

    size_t len = itr - *str + 1;
    char old_val = *itr;

    *itr = '\0';
    int val = (int)strtol(*str, NULL, 10);
    *itr = old_val;

    *str = itr + 1; //move the pointer past the parsed

    node* n = (node*) malloc(sizeof(node));
    n->type = NUMBER;
    n->val.num_val = val;
    return n;
}

void free_tree(node* root){
    node* child_itr = root->val.first_child;
    while (child_itr)
    {   
        node* temp = child_itr->next;
        free_tree(child_itr);
        child_itr = temp;
    }
    
}

/*Entry point*/
node* parse_json(char* str){ 
    char**itr = &str; 
    // to make the signature easier to cope with
    // and to make the changes done in pointer traversal transparent to the passed pointer
    skip_whitespaces(itr);
    // // printf("%c\n", **itr);
    if (**itr != '{') //malformed
        return NULL;
    // printf("starting parsing\n");
    node* res = parse_object(itr);
    return res;
}


node* get(node* src, char* name){
    if(!src || !(src->type != ARRAY && src->type != OBJECT) )
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
    char json[] = "{ \"name\" : \"alex\", \"friends\" : [\"joseph\"]}";
    node* res = parse_json(json);
    if (!res)
    {
        printf("invalid json\n");
    } else {
        printf("%s\n", res->val.first_child->next->val.first_child->val.str_val);
    }
    
}