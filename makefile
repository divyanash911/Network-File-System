# Right now only passing the number of servers as argument later on can also pass number of folders and files
main:
	python3 setup_ss.py $(n)
	python3 start_ss.py $(n)
clean:
	python3 setup_ss.py $(n) clean