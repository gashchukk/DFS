DIRS := client masterNode chunkNode chunkNode_copy chunkNode_copy2

.PHONY: clean compile

clean:
	@echo "Cleaning compilation files and directories..."
	@for dir in $(DIRS); do \
		echo "Cleaning $$dir..."; \
		rm -rf $$dir/bin $$dir/cmake-build-debug $$dir/*.o $$dir/*.out $$dir/*.log $$dir/*.tmp $$dir/*.DS_Store; \
	done
	@rm -rf chunkNode/chunks/ ;
	@rm -rf chunkNode_copy ;
	@rm -rf chunkNode_copy2 ;
	@rm -rf ./*.DS_Store;
	@echo "Cleanup completed."

compile:
	@echo "Compiling all compile.sh files in directories..."
	cp -r chunkNode chunkNode_copy; \
	cp -r chunkNode chunkNode_copy2; 
	@for dir in $(DIRS); do \
		if [ -f $$dir/compile.sh ]; then \
			echo "Compiling in $$dir..."; \
			(cd $$dir && bash compile.sh); \
		else \
			echo "No compile.sh found in $$dir. Skipping..."; \
		fi \
	done
	@echo "Compilation completed."
