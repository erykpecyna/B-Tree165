/*
 * 
 * You will need to write your B+Tree almost entirely from scratch. 
 * 
 * B+Trees are dynamically balanced tree structures that provides efficient support for insertion, deletion, equality, and range searches. 
 * The internal nodes of the tree direct the search and the leaf nodes hold the base data..
 * 
 * For a basic rundown on B+Trees, we will refer to parts of Chapter 10 of the textbook Ramikrishnan-Gehrke 
 * (all chapters and page numbers in this assignment prompt refer to the 3rd edition of the textbook).
 *
 * Read Chapter 10 which is on Tree Indexing in general. In particular, focus on Chapter 10.3 on B+Tree.
 */
#ifndef NODE_KEY_AMOUNT
#define NODE_KEY_AMOUNT 1020
#endif

#ifndef LEAF_KEY_AMOUNT
#define LEAF_KEY_AMOUNT 510
#endif

#ifndef BTREE_H
#define BTREE_H

#include "data_types.h"
#include "query.h"


/* 
Designing your C Structs for B+Tree nodes (Chapter 10.3.1)
How will you represent a B+Tree as a C Struct (or series of C structs that work together)? There are many valid ways to do this part of your design, and we leave it open to you to try and tune this as you progress through the project.
How will you account for a B+Tree node being an internal node or a leaf node? Will you have a single node type that can conditionally be either of the two types, or will you have two distinct struct types?
How many children does each internal node have? This is called the fanout of the B+Tree.
What is the maximum size for a leaf node? How about for an internal node?
What is the minimum threshold of content for a node, before it has to be part of a rebalancing?
*/

// TODO: here you will need to define a B+Tree node(s) struct(s)
typedef struct leaf {
    int occupancy;
    KEY_t keys[LEAF_KEY_AMOUNT];
    VAL_t vals[LEAF_KEY_AMOUNT];
    struct leaf *next;
} leaf;

//Type: 0 = children are nodes
//      1 = children are leaves
typedef struct node {
    int occupancy;
    unsigned short int leafChildren;
    KEY_t keys[NODE_KEY_AMOUNT];
    void *next_nodes;
} node;

//Some headers
void node_newchild(node *nd, void **newchild, int index);

/* The following are methods that can be invoked on B+Tree node(s).
 * Hint: You may want to review different design patterns for passing structs into C functions.
 */

/* FIND (Chapter 10.4)
This is an equality search for an entry whose key matches the target key exactly.
How many nodes need to be accessed during an equality search for a key, within the B+Tree? 
*/

//This is for the sole purpose of finding the proper key to use when splitting a node
//of the tree to ensure that order is maintained and the proper key is assigned.
KEY_t find_smallest_key(node* nd) {
    if(nd->leafChildren)
        return ((leaf*)nd->next_nodes)[0].keys[0];
    else return find_smallest_key(&(((node*)nd->next_nodes)[0]));
}

//Search the leaf with a binary search
int leaf_find(KEY_t key, leaf* ptr, VAL_t *target) {
    for(int i = 0; i < ptr->occupancy; i++)
        if(ptr->keys[i] == key) {
            *target = ptr->vals[i];
            return 1;
        }
    return 0;
}

//Recursively find the path to the target value using a binary search through the keys
int node_find(KEY_t key, node* ptr, VAL_t *target) {
    for(int i = 0; i < ptr->occupancy; i++)
        if(key < ptr->keys[i]) {
            if(ptr->leafChildren)
                return leaf_find(key, &(((leaf*)ptr->next_nodes)[i]), target);
            else return node_find(key, &(((node*)ptr->next_nodes)[i]), target);
        }
    
    //This is seperate from the for loop to avoid an out of bounds reference to ptr->keys
    if(ptr->leafChildren)
        return leaf_find(key, &(((leaf*)ptr->next_nodes)[ptr->occupancy]), target);
    else return node_find(key, &(((node*)ptr->next_nodes)[ptr->occupancy]), target);
    return 0;
}   

// TODO: here you will need to define FIND/SEARCH related method(s) of finding key-values in your B+Tree.


/* INSERT (Chapter 10.5)
How does inserting an entry into the tree differ from finding an entry in the tree?
When you insert a key-value pair into the tree, what happens if there is no space in the leaf node? What is the overflow handling algorithm?
For Splitting B+Tree Nodes (Chapter 10.8.3)
*/
void leaf_insert(leaf* lf, KEY_t newKey, VAL_t newVal, void** newchildentry) {
    if(lf->occupancy == LEAF_KEY_AMOUNT) {
        int splitind = lf->occupancy / 2;
        leaf *newchild = malloc(sizeof(leaf));
        newchild->occupancy = lf->occupancy - splitind;
        for(int i = splitind; i < lf->occupancy; i++) {
            newchild->keys[i - splitind] = lf->keys[i];
            newchild->vals[i - splitind] = lf->vals[i];
        }

        lf->occupancy = splitind;
        newchild->next = lf->next;
        lf->next = newchild;
        *newchildentry = newchild;

        //If new val belongs in split node, then insert
        if(newKey >= newchild->keys[0]) {
            newchild->occupancy++;
            for(int i = 0; i < newchild->occupancy; i++){
                if(newKey < newchild->keys[i]){
                    for(int j = newchild->occupancy - 1; j > i; j--) {
                        newchild->keys[j] = newchild->keys[j-1];
                        newchild->vals[j] = newchild->vals[j-1];
                    }
                    newchild->keys[i] = newKey;
                    newchild->vals[i] = newVal;
                    break;
                } else if(newKey == newchild->keys[i]) {
                    newchild->vals[i] = newVal;
                    newchild->occupancy--;
                    break;
                } else if (i == newchild->occupancy - 1) {
                    newchild->keys[i] = newKey;
                    newchild->vals[i] = newVal;
                }
            }   
            return;
        }
    }

    lf->occupancy++;
    for(int i = 0; i < lf->occupancy; i++){
        if(newKey < lf->keys[i]){
            for(int j = lf->occupancy - 1; j > i; j--) {
                lf->keys[j] = lf->keys[j-1];
                lf->vals[j] = lf->vals[j-1];
            }
            lf->keys[i] = newKey;
            lf->vals[i] = newVal;
            break;
        } else if(newKey == lf->keys[i]) {
            lf->vals[i] = newVal;
            lf->occupancy--;
            break;
        } else if (i == lf->occupancy - 1) {
            lf->keys[i] = newKey;
            lf->vals[i] = newVal;
        }
    }
}

//Handles any type of node splitting and returns the resulting node and its split brother
//ind is the index of the child that split in the original node
void handle_node_split(node* nd, void** newchildentry, int ind) {
    if(!(*newchildentry))
        return;
    
    //First we have to split the existing full node by splitting the extra nodes
    //then copying keys over
    //SPLIT
    int splitind = (nd->occupancy + 1) / 2;
    node *newnode = malloc(sizeof(node));
    newnode->occupancy = nd->occupancy - splitind;
    newnode->leafChildren = nd->leafChildren;
    if(nd->leafChildren)
        newnode->next_nodes = malloc((NODE_KEY_AMOUNT + 1) * sizeof(leaf));
    else newnode->next_nodes = malloc((NODE_KEY_AMOUNT + 1) * sizeof(node));

    for(int i = splitind; i <= nd->occupancy; i++){
        if(nd->leafChildren) {
            ((leaf*)newnode->next_nodes)[i - splitind] = ((leaf*)nd->next_nodes)[i];
            if(i < nd->occupancy)
                newnode->keys[i - splitind] = nd->keys[i];
        } else {
            ((node*)newnode->next_nodes)[i - splitind] = ((node*)nd->next_nodes)[i];
            if(i < nd->occupancy)
                newnode->keys[i - splitind] = nd->keys[i];
        }
    }
    nd->occupancy = splitind - 1;

    //INSERT NEWCHILD
    if(ind < splitind)
        node_newchild(nd, newchildentry, ind);
    else node_newchild(newnode, newchildentry, ind - splitind);

    *newchildentry = newnode;
}

//nd's indexth child split and made newchild
void node_newchild(node *nd, void **newchild, int index) {
    if(nd->occupancy == NODE_KEY_AMOUNT){
        handle_node_split(nd, newchild, index);
        return;
    }

    //If need to insert before end, move everything over
    for(int i = nd->occupancy + 1; i > index + 1; i--) {
        if(nd->leafChildren) {
            //Since keys is shorter than next_nodes, this if
            // avoids out of bounds indexing
            if(i <= nd->occupancy)
                nd->keys[i] = nd->keys[i-1];
            ((leaf*)(nd->next_nodes))[i]=((leaf*)(nd->next_nodes))[i-1];
        } else {
            if(i <= nd->occupancy)
                nd->keys[i] = nd->keys[i-1];
            ((node*)(nd->next_nodes))[i]=((node*)(nd->next_nodes))[i-1];
        }
    }
   
    //Insert the new child
    if(nd->leafChildren) {
        //Unless the child has been added at the end of the node, write the new
        //keys for split node
        if(index < nd->occupancy)
            nd->keys[index + 1] = nd->keys[index];
        ((leaf*)(nd->next_nodes))[index + 1] = *((leaf*)(*newchild));
        nd->keys[index] = ((leaf*)(*newchild))->keys[0];
    } else {
        if(index < nd->occupancy)
            nd->keys[index + 1] = nd->keys[index];
        ((node*)(nd->next_nodes))[index + 1] = *((node*)(*newchild));
        nd->keys[index] = find_smallest_key((node*)(*newchild));
    }
    nd->occupancy++;

    *newchild = NULL;
}

void node_insert(node* nd, KEY_t newKey, VAL_t newVal, void** newchildentry) {
    for(int i = 0; i <= nd->occupancy; i++) {
        if(i < nd->occupancy) {
            if(newKey < nd->keys[i]) {
                if(nd->leafChildren)
                    leaf_insert(&((leaf*)nd->next_nodes)[i], newKey, newVal, newchildentry);
                else node_insert(&((node*)nd->next_nodes)[i], newKey, newVal, newchildentry);
                if(*newchildentry)
                    node_newchild(nd, newchildentry, i);
                break;
            }
        } else {
            if(nd->leafChildren)
                leaf_insert(&((leaf*)nd->next_nodes)[i], newKey, newVal, newchildentry);
            else node_insert(&((node*)nd->next_nodes)[i], newKey, newVal, newchildentry);
            if(*newchildentry)
                node_newchild(nd, newchildentry, i);
            break;    
        }
    }
}


//FREE TREE
void free_node_rec(node* nd) {
    if(nd->leafChildren) {
        free((leaf*)nd->next_nodes);
        return;
    }
    else for(int i = 0; i <= nd->occupancy; i++)
        free_node_rec(&(((node*)nd->next_nodes)[i]));
    free((node*)nd->next_nodes);
}

// TODO: here you will need to define INSERT related method(s) of adding key-values in your B+Tree.


/* BULK LOAD (Chapter 10.8.2)
Bulk Load is a special operation to build a B+Tree from scratch, from the bottom up, when beginning with an already known dataset.
Why might you use Bulk Load instead of a series of inserts for populating a B+Tree? Compare the cost of a Bulk Load of N data entries versus that of an insertion of N data entries? What are the tradeoffs?
*/

// TODO: here you will need to define BULK LOAD related method(s) of initially adding all at once some key-values to your B+Tree.
// BULK LOAD only can happen at the start of a workload


/*RANGE (GRADUATE CREDIT)
Scans are range searches for entries whose keys fall between a low key and high key.
Consider how many nodes need to be accessed during a range search for keys, within the B+Tree?
Can you describe two different methods to return the qualifying keys for a range search? 
(Hint: how does the algorithm of a range search compare to an equality search? What are their similarities, what is different?)
Can you describe a generic cost expression for Scan, measured in number of random accesses, with respect to the depth of the tree?
*/

// TODO GRADUATE: here you will need to define RANGE for finding qualifying keys and values that fall in a key range.



#endif