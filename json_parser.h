
typedef enum{
    STRING,
    NUMBER,
    BOOL,
    NONE,
    ARRAY,
    OBJECT
} node_type;

typedef struct node node;

typedef union {
    int num_val; //for numbers, bools
    char* str_val; // for strings
    node* first_child; //for composite objects
} node_val;

typedef struct node{
    node_type type;
    node_val val;
    char* name;
    node* next;
    node* prev;
} node;



node* parse_key_val(char**);

node* parse_object(char**);

node* parse_array(char**);

node* parse_string(char**);

node* parse_number(char**);

void free_tree(node*);

node* parse_json(char*);