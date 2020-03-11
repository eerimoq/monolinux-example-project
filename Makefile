default:
	$(MAKE) -C app

run:
	$(MAKE) -C app run

clean:
	$(MAKE) -C app clean

docker-image:
	cd docker && docker build -t monolinux-example-project .
