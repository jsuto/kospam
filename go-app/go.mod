module kospam

go 1.23.7

replace kospam/smtpd => ./

require (
	github.com/jsuto/go-smtp v0.0.0-20250329193343-aa0eef933647
	github.com/stretchr/testify v1.10.0
	kospam/smtpd v0.0.0-00010101000000-000000000000
)

require (
	github.com/davecgh/go-spew v1.1.1 // indirect
	github.com/emersion/go-sasl v0.0.0-20200509203442-7bfe0ed36a21 // indirect
	github.com/pmezard/go-difflib v1.0.0 // indirect
	gopkg.in/yaml.v3 v3.0.1 // indirect
)
