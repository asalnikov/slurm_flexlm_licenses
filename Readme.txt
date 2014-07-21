Your need place this files into the catalogue
of Slurm 14 sources 
src/plugins/sched/backfill

You need change some headers in src/slurmctld/licenses.cpp and src/common
to remove static on mutex and license list manipulation function.

These objects are: 

pthread_mutex_t licenses_mutex;
int _license_find_rec(void *x, void *key);


