%.envrc: ./*.nix
	nix-shell -A shell --pure --run 'export -p > .envrc'
