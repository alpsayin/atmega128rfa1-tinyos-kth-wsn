/*
 * Copyright (c) 2008-2010 The Regents of the University  of California.
 * All rights reserved."
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the copyright holders nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <Shell.h>
#include <iprouting.h>

module RouteCmdP {
  uses interface ShellCommand;
  uses interface ForwardingTable;
} implementation {
  
  char *header = "destination\t\tgateway\t\tiface\n";
  struct {
    int ifindex;
    char *name;
  } ifaces[3] = {{0, "any"}, {1, "pan"}, {2, "ppp"}};

  char *ifnam(int ifidx) {
    int i;
    for (i = 0; i < sizeof(ifaces) / sizeof(ifaces[0]); i++) {
      if (ifaces[i].ifindex == ifidx) 
        return ifaces[i].name;
    }
    return NULL;
  }

  event char *ShellCommand.eval(int argc, char **argv) {
#define LEN (MAX_REPLY_LEN - (cur - buf))
    struct route_entry *entry;
    char *cur, *buf = call ShellCommand.getBuffer(MAX_REPLY_LEN);
    int i, n;

    entry = call ForwardingTable.getTable(&n);
    if (!buf || !entry)
      return NULL;

    cur = buf;
    memcpy(cur, header, strlen(header));
    cur += strlen(header);
    for (i = 0; i < n; i++) {
      if (entry[i].valid) {
        cur += inet_ntop6(&entry[i].prefix, cur, LEN) - 1;
        cur += snprintf(cur, LEN, "/%i\t\t", entry[i].prefixlen);
        cur += inet_ntop6(&entry[i].next_hop, cur, LEN) - 1;
        if (LEN < 6) continue;
        *cur++ = '\t'; *cur++ = '\t';
        strncpy(cur, ifnam(entry[i].ifindex), LEN);
        cur += 3;
        *cur++ = '\n';
        if (LEN < MAX_REPLY_LEN / 2) {
          call ShellCommand.write(buf, cur - buf);
          cur = buf;
        }
      }
    }
    if (cur > buf) 
      call ShellCommand.write(buf, cur - buf);
    return NULL;
  }
}
