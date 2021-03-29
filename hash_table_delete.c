int delete(userlinkedlist **users, userlinkedlist *userll)
{
    if (userll == NULL)
    {
        printf("User is NULL\n");
        return -1;
    }

    int index = hash(userll->username);

    if(users[index] == NULL)
    {
        printf("User doesn't exist.\n");
        return 0;
    }

    userlinkedlist * current_node = users[index];
    userlinkedlist * previous_node = NULL;

    while (current_node != NULL && strncmp(current_node->username, userll->username, 50) != 0)
    {
        previous_node = current_node;
        current_node = current_node->next;        
    }

    if (previous_node == NULL)
    {
        users[index] = NULL;
    }
    else
    {
        previous_node->next = current_node->next;
    }
    return 1;
}