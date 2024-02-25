/*******************************************************************************
 * Name        : bstree.c
 * Author      : Yuvan Rengifo
 * Pledge      : I pledege my honor that I have abided by the Stevens honor system
 ******************************************************************************/
#include "bstree.h"

void add_node(void* data, size_t dataS, tree_t* tree, int (*cmp)(void*, void*)) {
    // Allocate space for the new node
    node_t* newN = (node_t*)malloc(sizeof(node_t));

    // Allocate space for the data
    newN->data = malloc(dataS);

    if (newN->data == NULL) {
        free(newN); // Free the node to avoid memory leaks
        return;
    }

    // Copying data byte by byte
    for (size_t i = 0; i < dataS; ++i) {
        *((char*)newN->data + i) = *((char*)data + i);
    }

    newN->left = NULL; // left child
    newN->right = NULL; //right child

    // Find insertion of node
    if (tree->root == NULL) {     // New node becomes root
        tree->root = newN;
    } else {

        //Iterate to find correct place
        node_t* curr = tree->root;
        node_t* parent = NULL;
        while (curr != NULL) {
            parent = curr;
            int result = cmp(data, curr->data); // Compares current nodes value and new value

            if (result < 0) {
                curr = curr->left; // Go left 
            } else {
                curr = curr->right; // Go right.
            }
        }

        // Compare parent and new value to decide if its left or right child
        if (cmp(data, parent->data) < 0) {
            parent->left = newN;
        } else {
            parent->right = newN;
        }
    }
}

void print_tree(node_t* start, void (*prnt)(void*)){
     if (start == NULL) {
        return;
    }
    // Print the left subtree.
    print_tree(start->left, prnt);

    // Print start node
    prnt(start->data);

    // Print the right subtree.
    print_tree(start->right, prnt);
}


void freeMaBois(node_t* boi) {
    if (boi == NULL) {
        return; 
    }

    // free left and right subtrees
    freeMaBois(boi->left);
    freeMaBois(boi->right);

    // Free the node's data, then the node itself
    free(boi->data);
    free(boi);
}

void destroy(tree_t* tree) {
    if (tree == NULL) {
        return;
    }
    // free all the nodes
    freeMaBois(tree->root);
    //set the root to NULL
    tree->root = NULL;
}