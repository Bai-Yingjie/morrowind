def print_filed(reg64, high, low):
	field = reg64[-1-high:-1-low] + reg64[-1-low]
	if high == low:
		print '[%d]' % high
	else:
		print '[%d-%d]' % (high, low)
	print 'Bin:', field
	print 'Dec:', int(field, 2)
	print 'Hex:', hex(int(field, 2))
	print

def dr(val, *args):
	reg = bin(val)[2:]
	reg64 = '0' * (64 - len(reg)) + reg
	for arg in args:
		if type(arg) is list:
			high = arg[0]
			low = arg[1]
		else:
			high = low = arg
		print_filed(reg64, high, low)
