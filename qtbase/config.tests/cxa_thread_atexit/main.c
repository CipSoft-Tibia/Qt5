// Copyright (C) 2024 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include <stddef.h>

typedef void (*dtor_func) (void *);
int TEST_FUNC(dtor_func func, void *obj, void *dso_symbol);
int main()
{
    return TEST_FUNC(NULL, NULL, NULL);
}
