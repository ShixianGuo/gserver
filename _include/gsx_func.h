
#ifndef __GSX_FUNC_H__
#define __GSX_FUNC_H__

void Rtrim(char *string);
void Ltrim(char *string);

void gsx_init_setproctitle();
void gsx_setproctitle(const char *title);

void gsx_log_init();
void gsx_log_stderr(int err, const char *fmt, ...);
void gsx_log_error_core(int level, int err, const char *fmt, ...);
u_char *gsx_log_errno(u_char *buf, u_char *last, int err);
u_char *gsx_snprintf(u_char *buf, size_t max, const char *fmt, ...);
u_char *gsx_slprintf(u_char *buf, u_char *last, const char *fmt, ...);
u_char *gsx_vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args);

int gsx_init_signals();
void gsx_master_process_cycle();
int gsx_daemon();
void gsx_process_events_and_timers();

#endif