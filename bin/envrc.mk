%.envrc: ./*.nix
	nix-shell --pure --run 'export -p > .envrc'
