.PHONY: verify security native test snapshots assets diagrams firmware export package flash monitor clean

PIO_ENV ?= crowpanel_idf5

verify:
	python3 tools/verify_repo.py

security:
	python3 tools/security_audit.py

native: verify
	cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
	cmake --build build

test: native
	ctest --test-dir build --output-on-failure

snapshots: native
	./build/housecat_simulator previews/native
	python3 tools/build_previews.py

assets:
	python3 tools/generate_assets.py

# Requires Graphviz and CairoSVG for the controls card.
diagrams:
	@for file in docs/diagrams/*.dot; do \
		base=$${file%.dot}; \
		dot -Tsvg $$file -o $$base.svg; \
		dot -Tpng -Gdpi=180 $$file -o $$base.png; \
	done

firmware: verify
	python3 -m platformio run -e $(PIO_ENV)

export: firmware
	python3 tools/export_firmware.py --env $(PIO_ENV)

package: verify
	python3 tools/package_source.py --output ..

flash: verify
	python3 -m platformio run -e $(PIO_ENV) -t upload

monitor:
	python3 -m platformio device monitor -b 115200

clean:
	rm -rf build build-* .pio dist/housecat-*
