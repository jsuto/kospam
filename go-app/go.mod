module kospam

go 1.23.7

replace kospam/smtpd => ./

require github.com/jsuto/go-smtp v0.0.0-20250310051436-8a5d7dcc0969

require (
	github.com/emersion/go-sasl v0.0.0-20200509203442-7bfe0ed36a21 // indirect
	kospam/smtpd v0.0.0-00010101000000-000000000000 // indirect
)
