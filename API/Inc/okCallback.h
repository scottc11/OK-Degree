#pragma once

/**
 * @brief void *
 * If a pointer's type is void*, the pointer can point to any variable that's not declared with the const or volatile keyword. 
 * A void* pointer can't be dereferenced unless it's cast to another type.
 * A void* pointer can be converted into any other type of data pointer.
 */

void myFunc(char str[])
{
    // print the string
}

void executeFunc(void (*func)(char *), char param[])
{
    func(param);
}

int main() {

    // Generic Callback
    executeFunc(myFunc, "This is Still actually working... \n");

    // LAMBDA Style Callback
    executeFunc([](char data[]){/*print(data);*/}, "This is Still actually working... \n");
}
