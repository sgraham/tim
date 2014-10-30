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

#include <algorithm>

LPWSTR FindStartOfSubCommand(LPWSTR orig_command, bool* want_avg) {
  int num_args;
  LPWSTR* args = CommandLineToArgvW(orig_command, &num_args);
  if (num_args < 2) {
    wprintf(L"tim: Could not get subprocess command line.\n");
    exit(1);
  }
  // If the binary is named 'timavg' rather than 'tim' we run multiple times.
  *want_avg = wcscmp(args[0], L"timavg") == 0;
  LPWSTR ret = orig_command + wcslen(args[0]);
  if (orig_command[0] == L'"')
    ret += 2; // Opening and closing quote.
  LocalFree(args);
  while (*ret == L' ')
    ++ret;
  return ret;
}

struct Result {
  ULONGLONG elapsed;
  LARGE_INTEGER qpc_elapsed_microseconds;
  DWORD exit_code;
};

Result Run(LPWSTR command) {
  STARTUPINFO startup_info = { sizeof(STARTUPINFO) };
  PROCESS_INFORMATION process_info;
  GetStartupInfo(&startup_info);
  Result result;

  ULONGLONG start_time = GetTickCount64();

  LARGE_INTEGER qpc_frequency;
  LARGE_INTEGER qpc_start_time, qpc_end_time;
  QueryPerformanceFrequency(&qpc_frequency);
  QueryPerformanceCounter(&qpc_start_time);

  if (!CreateProcess(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL,
                     &startup_info, &process_info)) {
    wprintf(L"tim: Could not spawn subprocess, command line:\n"
            L"%ws\n"
            L"You may need to prefix the command with \"cmd /c \".\n",
            command);
    exit(1);
  }
  CloseHandle(process_info.hThread);
  WaitForSingleObject(process_info.hProcess, INFINITE);
  GetExitCodeProcess(process_info.hProcess, &result.exit_code);
  CloseHandle(process_info.hProcess);

  QueryPerformanceCounter(&qpc_end_time);
  result.qpc_elapsed_microseconds.QuadPart =
      qpc_end_time.QuadPart - qpc_start_time.QuadPart;
  result.qpc_elapsed_microseconds.QuadPart *= 1000000;
  result.qpc_elapsed_microseconds.QuadPart /= qpc_frequency.QuadPart;

  ULONGLONG end_time = GetTickCount64();
  result.elapsed = end_time - start_time;

  wprintf(L"\nreal: %lldm%.03fs\nqpc: %lldus\n",
          result.elapsed / (60 * 1000),
          (result.elapsed % (60 * 1000)) / 1000.0,
          result.qpc_elapsed_microseconds.QuadPart);
  return result;
}

int main() {
  bool want_avg = false;
  LPWSTR command = FindStartOfSubCommand(GetCommandLine(), &want_avg);
  if (want_avg) {
    Result runs[7];
    for (int i = 0; i < ARRAYSIZE(runs); ++i)
      runs[i] = Run(command);
    std::sort(runs,
              &runs[ARRAYSIZE(runs)],
              [](const Result& a, const Result& b) {
      return a.qpc_elapsed_microseconds.QuadPart <
             b.qpc_elapsed_microseconds.QuadPart;
    });
    ULONGLONG elapsed_sum = 0;
    LARGE_INTEGER elapsed_qpc_sum = {0};
    for (int i = 1; i < ARRAYSIZE(runs) - 1; ++i) {
      elapsed_sum += runs[i].elapsed;
      elapsed_qpc_sum.QuadPart += runs[i].qpc_elapsed_microseconds.QuadPart;
    }
    elapsed_sum /= ARRAYSIZE(runs) - 2;
    elapsed_qpc_sum.QuadPart /= ARRAYSIZE(runs) - 2;
    wprintf(L"avg = %lldm%0.3fs\n      %lldus\n",
          elapsed_sum / (60 * 1000),
          (elapsed_sum % (60 * 1000)) / 1000.0,
          elapsed_qpc_sum.QuadPart);
  } else {
    Result result = Run(command);
    exit(result.exit_code);
  }
}
