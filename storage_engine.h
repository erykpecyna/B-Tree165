/*
 * The storage engine functions prefixed with `wrapper`, map the corresponding k-v operations 
 * of the k-v DSL to actual B+Tree functions invoked on your B+Tree.
 */
#ifndef STORAGE_ENGINE_H
#define STORAGE_ENGINE_H

#include "data_types.h"
#include "query.h"
#include "btree.h"

/*
 * A struct to store the context of your storage engine.
 * You may wish to extend this struct, to not only store your B+Tree, but also store additional state.
 * 
 * 1. You will need to decide how to hold a B+Tree in your storage context. 
 *      Hint, you will need to modify the storage context struct, 
 *      while thinking about memory management and pointers for accessing your B+Tree.
 */
typedef struct storageContext {
    // TODO: you hold a pointer here to find your B+Tree
    unsigned short int small_leaf;
    void *head;
} STORAGECXT_t;


#ifdef TESTING
//The following are functions that print the structure of a tree to aid in testing.
void printleaf(leaf *lf) {
    printf("LEAF VALUES %d\n", lf->occupancy);
    for(int i = 0; i < lf->occupancy; i++)
        printf("%d ", lf->vals[i]);
    if(lf->next)
        printf("The first value in the next leaf is %d\n", lf->next->vals[0]);
    printf("\n");
}

void printnode(node *nd, int level) {
    printf("NODE LVL %d WITH %d KEYS\n", level, nd->occupancy);
    for(int i = 0; i < nd->occupancy; i++)
        printf("%d ", nd->keys[i]);
    printf("\n");

    for(int i = 0; i <= nd->occupancy; i++)
        if(nd->leafChildren)
            printleaf(&((leaf*)(nd->next_nodes))[i]);
        else printnode(&((node*)(nd->next_nodes))[i], level + 1);
}
#endif

void free_tree(STORAGECXT_t **stg) {
    if(*stg) {
        if((*stg)->small_leaf) {
            free((leaf*)((*stg)->head));
        }
        else free_node_rec((node*)((*stg)->head));
        
        free(*stg);
    }
}

/*
 * The following are wrapper functions which are entry points into your storage engine.
 * 
 * You will need to make sure these call your B+Tree to do actual data acccess operations.
 * Your B+Tree will need multiple supporting methods in order to be complete.
 * It is up to you to design: define and declare the additional functions B+t
 */

/*
 *  Get looks for a `targetKey`, and will give back the corresponding key in foundVal, if it exists.
 * 
 *  @Params: 
 *      storageEngine   - the storage engine to operate on
 *      targetKey       - the target key to search for
 *      foundVal        - declared outside the function invocation. should hold any found value.
 * 
 *  Returns:
 *       0 for no result, 
 *       1 if a value exists for `targetKey` in which case `foundVal` is populated
 * 
 */
int wrapperGet(STORAGECXT_t **storageEngine, KEY_t targetKey, VAL_t *foundVal){
    // TODO: finish this by running the find operation on your B+Tree.
    //      You will need to define and declare a method(s) to run find on your B+Tree.
    if(!(*storageEngine))
        return 0;
    
    STORAGECXT_t *stg = *storageEngine;

    if(stg->small_leaf)
        return leaf_find(targetKey, stg->head, foundVal);  
    else
        return node_find(targetKey, stg->head, foundVal);
}



/*
 *  Put sets a key value pair. 
 *  If a key does not exist previously in the storage engine, it should be inserted.
 *  If the key exists previously, the old value should be overwritten with the newly specified value.
 * 
 *  @Params: 
 *      storageEngine   - the storage engine to operate on
 *      key             - key to add
 *      val             - val to add
 * 
 *  Returns:
 *       0 for failed, 
 *       1 if the k,v pair addition succeeded
 * 
 */

int wrapperPut(STORAGECXT_t **storageEngine, KEY_t key, VAL_t val){
    // TODO: finish this by running the `insert` or `update` operation on your B+Tree.
    //      You will need to define and declare a method(s) to run `insertions` and `updates` on your B+Tree.
    //      Consider: will you treat `insertions` and `updates` as the same algorithm?
    //      Consider: how does the design of these B+Tree algorithms compare to `find`?
    if(!(*storageEngine)) {
        *storageEngine = malloc(sizeof(STORAGECXT_t));
        (*storageEngine)->small_leaf = 1;
        leaf *newleaf = malloc(sizeof(leaf));

        if(newleaf == NULL)
            return 0;

        newleaf->occupancy = 0;
        newleaf->next = NULL;
        (*storageEngine)->head = newleaf;
    }
    void *newchild = NULL;
    void *nd = (*storageEngine)->head;

    if((*storageEngine)->small_leaf)
        leaf_insert((leaf*)nd, key, val, &newchild);
    else node_insert((node*)nd, key, val, &newchild);
    if(newchild) {
        if((*storageEngine)->small_leaf) {
            node *newnode = malloc(sizeof(node));
            newnode->occupancy = 1;
            newnode->leafChildren = 1;
            newnode->keys[0] = ((leaf*)newchild)->keys[0];
            newnode->next_nodes = malloc((NODE_KEY_AMOUNT + 1) * sizeof(node));
            ((leaf*)newnode->next_nodes)[0] = *((leaf*)nd);
            ((leaf*)newnode->next_nodes)[1] = *((leaf*)newchild);
            (*storageEngine)->head = newnode;
            (*storageEngine)->small_leaf = 0;
        } else {
            //Allocate a new head for the tree
            node *newhead = malloc(sizeof(node));
            newhead->occupancy = 1;
            newhead->leafChildren = 0;
            //This gets the smallest value in newchild because of the way the nodes are split
            newhead->keys[0] = find_smallest_key((node*)newchild);
            newhead->next_nodes = malloc((NODE_KEY_AMOUNT + 1) * sizeof(node));
            //Add the old node and the one split off of it into the new head
            ((node*)newhead->next_nodes)[0] = *((node*)(*storageEngine)->head);
            ((node*)newhead->next_nodes)[1] = *((node*)newchild);
            (*storageEngine)->head = newhead;
        }
    }


    //Will only be used if built for testing
    #ifdef TESTING
    printf("\n\n\n");
    if((*storageEngine)->small_leaf)
        printleaf((leaf*)(*storageEngine)->head);
    else printnode((node*)(*storageEngine)->head, 0);
    #endif
    
    return 1;
}

/*
 *  Put sets a key value pair. 
 *  If a key does not exist previously in the storage engine, it should be inserted.
 *  If the key exists previously, the old value should be overwritten with the newly specified value.
 * 
 *  @Params: 
 *      storageEngine      - the storage engine to operate on
 *      lowKey             - key to add
 *      highKey            - val to add
 *      rangeResult        - results, which if found, will be populated 
 *                              (see the query.h definition for more details)
 *  Returns:
 *       0 for no result, 
 *       non-zero indicating the number of the qualifying entries, held inside `rangeResult`'s keys and values
 * 
 */
// returns 0 for no result
// returns non-zero for the length of the qualifying entries
int wrapperRange(STORAGECXT_t **storageEngine, KEY_t lowKey, KEY_t highKey, RANGE_RESULT_t **rangeResult){
    // TODO GRADUATE: 
    //      You will need to define and declare a method(s) to run 
    (void) storageEngine;
    (void) lowKey;
    (void) highKey;
    (void) rangeResult;
    return 0;
}

#endif