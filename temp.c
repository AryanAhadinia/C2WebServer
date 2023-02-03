char * parse_path(QueryParameters * query_parameters, char * path) {
    char * path_copy = malloc(strlen(path) + 1);
    strcpy(path_copy, path);
    char * path_without_query = malloc(strlen(path) + 1);   /// pending
    
    strcpy(path_without_query, path);
    char * query_start = strstr(path_copy, "?");
    if (query_start != NULL) {
        int path_without_query_length = query_start - path_copy;

        char * path_without_query = malloc(strlen(path) + 1);   /// pending

        path_without_query[path_without_query_length] = '\0';
        char * query = query_start + 1;
        char * key = strtok(query, "=");
        char * value = strtok(NULL, "&");
        query_parameters -> keys = malloc(sizeof(char * ));  /// pending
        query_parameters -> values = malloc(sizeof(char * ));
        query_parameters -> keys[0] = key;
        query_parameters -> values[0] = value;
        query_parameters -> num_params = 1;
        while (value != NULL) {
            key = strtok(NULL, "=");
            value = strtok(NULL, "&");
            if (key != NULL && value != NULL) {
                query_parameters -> keys = realloc(query_parameters -> keys, sizeof(char * ) * (query_parameters -> num_params + 1));
                query_parameters -> values = realloc(query_parameters -> values, sizeof(char * ) * (query_parameters -> num_params + 1));
                query_parameters -> keys[query_parameters -> num_params] = key;
                query_parameters -> values[query_parameters -> num_params] = value;
                query_parameters -> num_params++;
            }
        }
    }
    else{
    }
    // free(path_copy);
    return path_without_query;
}



  // char * header_copy = malloc(header_length + 1);
    // strncpy(header_copy, header, header_length);
    // header_copy[header_length] = '\0';
    char * key = strtok(header_copy, ":");
    char * value = strtok(NULL, "\r");
    http_header -> keys = malloc(sizeof(char * ));
    http_header -> values = malloc(sizeof(char * ));
    http_header -> keys[0] = key;
    http_header -> values[0] = value + 1;
    http_header -> num_headers = 1;
    while (value != NULL) {
        key = strtok(NULL, ":");
        value = strtok(NULL, "\r");
        if (key != NULL && value != NULL) {
            key = key + 1;
            value = value + 1;
            http_header -> keys = realloc(http_header -> keys, sizeof(char * ) * (http_header -> num_headers + 1));
            http_header -> values = realloc(http_header -> values, sizeof(char * ) * (http_header -> num_headers + 1));
            http_header -> keys[http_header -> num_headers] = key;
            http_header -> values[http_header -> num_headers] = value;
            http_header -> num_headers++;
        }
    }
}
