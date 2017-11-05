import os
import subprocess
import shlex
import time
from timeit import default_timer as timer

class Test:
	def __init__(self, isPeloton, setup_script, run_script, output):
		self.isPeloton = isPeloton
		self.setup_script = setup_script
		self.run_script = run_script
		self.output = output

	def cleanup(self):
		#self.proc1.terminate()
		cmd = "rm " + self.output
		#os.system(cmd)

	def start_postgres_server(self):
		#path_to_build = "/Users/prashasthiprabhakar/Documents/2017/LLVM_UDF_CAPSONE/peloton/build"
		#os.chdir(path_to_build)

		cmd = "pg_ctl -D /usr/local/var/postgres -l /usr/local/var/postgres/server.log start > /dev/null &"
		args = shlex.split(cmd)
		# Start the server
		self.proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
		#print "Process ID: ", self.proc.pids
	def setup_udf_test(self):
		cmd = 'echo "\i %s" | psql postgres > %s 2>&1' % (self.setup_script, self.output)
		exit_code = os.system(cmd)
		return exit_code

	def run_udf(self):
		cmd = 'echo "\i %s" | psql postgres > %s 2>&1' % (self.run_script, self.output)
		#args = shlex.split(cmd)

		#self.proc1 = subprocess.Popen(args, shell=True)
		exit_code = os.system(cmd)
		return exit_code

test_script = "/Users/prashasthiprabhakar/Documents/2017/LLVM_UDF_CAPSONE/peloton/script/testing/dml/udf_postgres_setup.sql"
output = "/tmp/psql_out-1"
run_script = "/Users/prashasthiprabhakar/Documents/2017/LLVM_UDF_CAPSONE/peloton/script/testing/dml/udf_postgres_invocation.sql"

p_test = Test(False, test_script, run_script, output)
#p_test.start_postgres_server()

print("Sleeping for 5 seconds...")
# sleep for 5 seconds
time.sleep(5)

start = timer()
exit_code = p_test.setup_udf_test()
end = timer()
print "Time taken setup: ", end-start
print "exit_code: ", exit_code

start = timer()
#p_test.run_udf()
end = timer()
print "Time taken: ", end-start

print "cleaning..."
# last thing to be done
p_test.cleanup()

print "done"









