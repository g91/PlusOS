C := $(wildcard kernel/*/*.c kernel/*/*/*.c)
probe:
	@echo C=$(words $(C)) first=$(firstword $(C))
