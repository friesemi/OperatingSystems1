def mainFunction():
	import sys
	from random import seed
	from random import randint
	import time
	seed(time.time())

	string = ""
	letter = 0
	saveToFile(letter, string, "first.txt")
	saveToFile(letter, string, "second.txt")
	saveToFile(letter, string, "third.txt")
	
	mulValue = 1

	for _ in range(2):
		value = randint(1, 42)
		mulValue = value*mulValue
		print(value)
	print(mulValue)


def saveToFile(letter, string, filename):
	import sys
	from random import randint

	f = open(filename, "w+")
	for _ in range(10):
		letter = randint(97, 122)
		string += str(chr(letter))

	string += "\n"
	sys.stdout.write(string)
	f.write(string)
	f.close()

mainFunction()