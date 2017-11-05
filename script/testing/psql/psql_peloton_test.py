import os
import subprocess
import shlex
import time
from timeit import default_timer as timer

class Test:
	def __init__(self, isPeloton, port, setup_script, run_script, output1, output2):
		self.isPeloton = isPeloton
		self.port = port
		self.setup_script = setup_script
		self.run_script = run_script
		self.output1 = output1
		self.output2 = output2

	def cleanup(self):
		cmd = "rm " + self.output1
		#os.system(cmd)

	def start_peloton_server(self):
		#path_to_build = "/Users/prashasthiprabhakar/Documents/2017/LLVM_UDF_CAPSONE/peloton/build"
		#os.chdir(path_to_build)

		cmd = "/Users/prashasthiprabhakar/Documents/2017/LLVM_UDF_CAPSONE/peloton/build/bin/peloton -port %s > /dev/null &" % (self.port)
		args = shlex.split(cmd)
		# Start the server
		self.proc = subprocess.Popen(args)

		#print "Process ID: ", self.proc.pid

	def setup_udf_test(self):
		cmd = 'echo "\i %s" | psql sslmode=disable -U postgres -h localhost -p %s > %s 2>&1' % (self.setup_script, self.port, self.output1)
		exit_code = os.system(cmd)
		print "exit_code", exit_code
		return exit_code

	def run_udf(self):
		print " Inside run _udf"
		cmd = 'echo "\i %s" | psql sslmode=disable -U postgres -h localhost -p %s > %s 2>&1' % (self.run_script, self.port, self.output2)
		print cmd
		exit_code = os.system(cmd)
		'''args = shlex.split(cmd)
		self.proc1 = subprocess.Popen(args, shell=True)
		self.proc1.wait()'''

		#print "Exit_code", exit_code
		#return exit_code

test_script = "/Users/prashasthiprabhakar/Documents/2017/LLVM_UDF_CAPSONE/peloton/script/testing/dml/udf_peloton_setup.sql"
#test_script = "/Users/prashasthiprabhakar/Documents/2017/LLVM_UDF_CAPSONE/peloton/script/testing/dml/peloton_try.sql"
output1 = "/tmp/psql_setup_out"
output2 = "/tmp/psql_invocation_out"
port = "15721"
run_script = "/Users/prashasthiprabhakar/Documents/2017/LLVM_UDF_CAPSONE/peloton/script/testing/dml/udf_peloton_invocation.sql"

p_test = Test(True, port, test_script, run_script, output1, output2)
#p_test.start_peloton_server()

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
print "Time taken UDF: ", end-start

print "cleaning..."
# last thing to be done
p_test.cleanup()

print "done"


