To iniate the sphinx build, do the following:

	% cd docs
 
	% doxygen

	% mkdir _static


To rebuild the sphinx docs do the

	% make html


For the sphinx-build, you can use:

sphinx-build -b html -Dbreathe_projects.kord-api=xml . _build/html

