update:
    cd breadboard && make update

update-release:
    cd breadboard && make update-release

build:
    cd breadboard/pico && make clean && make build

build-all: 
    cd breadboard/pico && make build-all

release:
    cd breadboard && make release

publish: release
    echo "Releasing version $(VERSION)"
    gh release create "$(VERSION)" --draft --prerelease --title "$(VERSION)-beta" --notes-file release-notes.md


