project('Promises', 'cpp',
	default_options : ['warning_level=3', 'cpp_std=c++2a']
)

cpp = meson.get_compiler('cpp')

executable('Promises',
	sources: [
		'Promises.cpp',
		'Tests.cpp',
	],
	dependencies: [
		cpp.find_library('be')
	],
)

executable('Credentials',
	sources: [
		'CredentialsApp.cpp',
		'CredentialsWindow.cpp',
	],
	dependencies: [
		cpp.find_library('be')
	],
)
