module kospam

go 1.23.7

replace kospam/smtpd => ./

require (
	github.com/jsuto/go-smtp v0.0.0-20250405160823-4f4fb785027f
	github.com/redis/go-redis/v9 v9.7.3
	github.com/stretchr/testify v1.10.0
	kospam/smtpd v0.0.0-00010101000000-000000000000
)

require (
	github.com/cespare/xxhash/v2 v2.2.0 // indirect
	github.com/davecgh/go-spew v1.1.1 // indirect
	github.com/dgryski/go-rendezvous v0.0.0-20200823014737-9f7001d12a5f // indirect
	github.com/emersion/go-sasl v0.0.0-20200509203442-7bfe0ed36a21 // indirect
	github.com/pmezard/go-difflib v1.0.0 // indirect
	gopkg.in/yaml.v3 v3.0.1 // indirect
)
