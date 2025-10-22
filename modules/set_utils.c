
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "ADTSet.h"
#include "state.h"
#include "set_utils.h"

Pointer set_find_eq_or_greater(Set set, Pointer value) {
    
    if (set_find(set, value) != NULL) {
        return value;
    }
    else {
        set_insert(set, value);
        if (set_next(set,set_find_node(set, value)) == SET_EOF) {
            return NULL;
        }
        else {
            return set_node_value(set,set_next(set,set_find_node(set, value)));
        }
    }
}

Pointer set_find_eq_or_smaller(Set set, Pointer value) {

    if (set_find(set, value) != NULL)
        return set_find(set, value);
    else {
        set_insert(set, value);
        if (set_previous(set,set_find_node(set, value)) == SET_EOF)
            return NULL;
        else 
            return set_node_value(set,set_previous(set,set_find_node(set, value)));
    }

}