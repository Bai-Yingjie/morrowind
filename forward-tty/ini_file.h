#ifndef __INIFILE_H__
#define __INIFILE_H__

int write_conf_value(const char *key, char *value, const char *file);
int read_conf_value(const char *key, char *value, const char *file);

#endif
