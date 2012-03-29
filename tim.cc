// Copyright 2012 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

LPWSTR FindStartOfSubCommand(LPWSTR orig_command) {
  int num_args;
  LPWSTR* args = CommandLineToArgvW(orig_command, &num_args);
  if (num_args < 2) {
    wprintf(L"tim: Could not get subprocess command line.\n");
    exit(1);
  }
  LPWSTR ret = orig_command + wcslen(args[0]);
  if (orig_command[0] == L'"')
    ret += 2; // Opening and closing quote.
  LocalFree(args);
  while (*ret == L' ')
    ++ret;
  return ret;
}

int main() {
  LPWSTR command = FindStartOfSubCommand(GetCommandLine());

  STARTUPINFO startup_info = { sizeof(STARTUPINFO) };
  PROCESS_INFORMATION process_info;
  DWORD exit_code;
  GetStartupInfo(&startup_info);

  ULONGLONG start_time = GetTickCount64();
  if (!CreateProcess(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL,
                     &startup_info, &process_info)) {
    wprintf(L"tim: Could not spawn subprocess, command line:\n%ws\n", command);
    exit(1);
  }
  CloseHandle(process_info.hThread);
  WaitForSingleObject(process_info.hProcess, INFINITE);
  GetExitCodeProcess(process_info.hProcess, &exit_code);
  CloseHandle(process_info.hProcess);
  ULONGLONG end_time = GetTickCount64();
  ULONGLONG elapsed = end_time - start_time;
  wprintf(L"\nreal: %lldm%.03fs\n",
          elapsed / (60 * 1000),
          (elapsed % (60 * 1000)) / 1000.0);
  exit(exit_code);
}