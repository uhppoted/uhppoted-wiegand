.PHONY: build
.PHONY: build-all
.PHONY: release

update:
	echo "Nothing to do"

update-release:
	echo "Nothing to do"

build:
	cd pico && make clean && make build

build-all: 
	cd pico && make build-all

release:
	echo "Nothing to do"

publish: release
	echo "Releasing version $(VERSION)"
	gh release create "$(VERSION)" --draft --prerelease --title "$(VERSION)-beta" --notes-file release-notes.md


