DIRS := client masterNode chunkNode

.PHONY: clean compile

clean:
	@echo "Cleaning compilation files and directories..."
	@for dir in $(DIRS); do \
		echo "Cleaning $$dir..."; \
		rm -rf $$dir/bin $$dir/cmake-build-debug $$dir/*.o $$dir/*.out $$dir/*.log $$dir/*.tmp $$dir/*.DS_Store; \
	done
	@rm -rf chunkNode/chunks/ ;
	@echo "Cleanup completed."

compile:
	@echo "Compiling all compile.sh files in directories..."
	@for dir in $(DIRS); do \
		if [ -f $$dir/compile.sh ]; then \
			echo "Compiling in $$dir..."; \
			(cd $$dir && bash compile.sh); \
		else \
			echo "No compile.sh found in $$dir. Skipping..."; \
		fi \
	done
	@echo "Compilation completed."
