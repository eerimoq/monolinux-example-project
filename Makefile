.PHONY: test

default:
	$(MAKE) -C app

run:
	$(MAKE) -C app run

clean:
	$(MAKE) -C app clean

test:
	python3 test/test.py $(ARGS)

docker-image:
	cd docker && docker build -t monolinux-example-project .

docker-image-tag-and-push:
	docker tag monolinux-example-project:latest eerimoq/monolinux-example-project:$(TAG)
	docker push eerimoq/monolinux-example-project:$(TAG)
