typedef struct{
    int cell;
    int num;
}Item;

typedef struct ListNode{
    Item this;
    struct ListNode *next;
    struct ListNode *prev;
}ListNode;

typedef struct{
    ListNode *head;
    ListNode *tail;
    int len;
}List;

List * init_list(void);
ListNode* newNode(Item this);
void insert_head(List* list, Item this);
Item pop_head(List* list);
