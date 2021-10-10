//-------------------------------------------------------------------------------
// MIT License
//
// Copyright (c) 2020 Erukaron
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//-------------------------------------------------------------------------------
#include "COM_HEAD.h"
#include "stdio.h"
#include "LOGIN.H"
//-------------------------------------------------------------------------------

/*
 * Filedescription USERCATA.LOG:
 *
 * (1 Byte 0 .. 1)
 * First nibble is priority level (0 is highest, f is lowest)
 * Second nibble is user number (0..f)
 *
 * (12 Bytes 1 .. 13)
 * Username in ascii format
 *
 * (32 Bytes 14 .. 46)
 * Password SHA3-256 encoded
 */

//-------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------
const string USERNAME_PROMPT = "login: ";
const string PASSWORD_PROMPT = "password: ";
#define USERNAME_LENGTH 12
#define PASSWORD_LENGTH 32
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// Prototypes
//-------------------------------------------------------------------------------
string sha3_256(string data);
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// Variables
//-------------------------------------------------------------------------------
char EnteredUsername[USERNAME_LENGTH] = {0};
char EnteredPassword[PASSWORD_LENGTH] = {0};
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------
void main(char argc, char *argv)
{
    printf(USERNAME_PROMPT);
    readln(USERNAME_LENGTH, &EnteredUsername);
    printf(EnteredUsername);
    printf(PASSWORD_PROMPT);
    readln_pw(PASSWORD_LENGTH, &EnteredPassword, '*');
    printf(EnteredPassword);

    //EnteredPassword = sha3_256(EnteredPassword);

    // Check if username/password combination are listed in the user catalog
}

string sha3_256(string data)
{
    return data;
}
//-------------------------------------------------------------------------------