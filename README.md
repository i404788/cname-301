# CName301
A universal adapter for CNAME to HTTP(S) redirects. Use either raw domain names or base16 encoded URLs to redirect using DNS only.

It's writtin in using Cpp-HTTPd in order to achieve high performance, but it can be implemented in any language.

## Usage
* Set your DNS A record for `*.r.devd.pw` to the CName301 instance
  * Skip this if you already have a CName301 instance
* Then you can use it by setting a DNS CNAME record to `{redirect-domain}.r.devd.pw` (e.g. `devdroplets.com.r.devd.pw`)
  * If you want to use a full URL encode it to hex first

## Dependencies
* [cpp-httpd](https://git.devdroplets.com/root/cpp-http)

## How does it work.
CName301 is extremely simple in design, and could be implemented in any language with relative ease (even bash I suspect).
Here is the pseudo code:
```

our_domain = "r.devd.pw"
on_connection(() => {
	domain = get_http_fqdn()
	domain = domain.replace("." + our_domain, "")
	if domain.includes('.') {
		respond(301, domain)
	} else {
		domain = parse_base16(domain).unwrap()
		respond(301, domain)
	}
})

```
