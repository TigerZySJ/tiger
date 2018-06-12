#pragma once
#ifndef CODEGEN_H
#define CODEGEN_H
#include <stdio.h>
#include <string.h>
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "assem.h"
AS_instrList F_codegen(F_frame frame, T_stmList stmList);
#endif // !CODEGEN_H
