#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>


// Choose a user environment to run and run it.
void
sched_yield(void)
{
	struct Env *idle;
	int i = 0;

	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING) and never choose an
	// idle environment (env_type == ENV_TYPE_IDLE).  If there are
	// no runnable environments, simply drop through to the code
	// below to switch to this CPU's idle environment.

	// LAB 4: Your code here.
	
	// Find index of current environment if it exists
	if(curenv != NULL)
	for (i = 0; i < NENV; i++)
	{
		if(&envs[i] == curenv)
			break;
	}

	// If curenv was not found, check env[0] and set i = 0 if
	// it cannot be run
	if(i == NENV || curenv == NULL)
	{
		if(envs[0].env_type != ENV_TYPE_IDLE &&
			 envs[0].env_status == ENV_RUNNABLE)
		{
			env_run(&envs[0]);
		}

		i = 0;
	}

	// Starting at the index 1 past the current env in the circular
	// buffer (hence mod NENV) find a runnable non-idle env
	int j;
	for (j = (i+1)%NENV; j != i; j = (j+1) % NENV)
	{
		if (envs[j].env_type != ENV_TYPE_IDLE &&
			 envs[j].env_status == ENV_RUNNABLE)
		{
			env_run(&envs[j]);
		}
	}

	// Choose the current env if it is runnable, otherwise fall through
	if(curenv && 
			(curenv->env_status == ENV_RUNNABLE ||
			(curenv->env_status == ENV_RUNNING)))
	{
		env_run(curenv);
	}

	// For debugging and testing purposes, if there are no
	// runnable environments other than the idle environments,
	// drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if (envs[i].env_type != ENV_TYPE_IDLE &&
		    (envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING))
			break;
	}
	if (i == NENV) {
		cprintf("No more runnable environments!\n");
		while (1)
			monitor(NULL);
	}

	// Run this CPU's idle environment when nothing else is runnable.
	idle = &envs[cpunum()];
	if (!(idle->env_status == ENV_RUNNABLE || idle->env_status == ENV_RUNNING))
		panic("CPU %d: No idle environment!", cpunum());
	env_run(idle);
}
