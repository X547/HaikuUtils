project('FreeTypeHarfBuzzDemo', 'cpp')

cpp = meson.get_compiler('cpp')

dep_be = cpp.find_library('be')
dep_freetype2 = dependency('freetype2')
dep_harfbuzz = dependency('harfbuzz')

executable(
	'FreeTypeHarfBuzzDemo',
	'Main.cpp',
	dependencies: [
		dep_be,
		dep_freetype2,
		dep_harfbuzz,
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)
