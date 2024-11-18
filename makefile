DIRS := client masterNode chunkNode

.PHONY: clean

clean:
	@echo "Cleaning compilation files and directories..."
	@for dir in $(DIRS); do \
		echo "Cleaning $$dir..."; \
		rm -rf $$dir/bin $$dir/cmake-build-debug $$dir/*.o $$dir/*.out $$dir/*.log $$dir/*.tmp; \
	done
	@echo "Cleanup completed."
