project('MinApp', 'cpp')

cpp = meson.get_compiler('cpp')
dep_be = cpp.find_library('be')

executable('MinApp',
	'MinApp.cpp',
	dependencies: [
		dep_be,
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)
