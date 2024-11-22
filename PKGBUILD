# Maintainer: Raj Acharya <razzacharya6@gmail.com>
pkgname=razzshell
pkgver=1.0.1
pkgrel=1
pkgdesc="A custom Unix shell with unique features"
arch=('x86_64')
url="https://github.com/rajacharya987/razzshell"
license=('MIT')
depends=('readline')
source=("$pkgname-$pkgver.tar.gz::$url/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('27f705bc5e86ffc7527d31862972f51a0df2c4ef2da12f55e62074dd7d506c07')

build() {
    cd "$srcdir/$pkgname-$pkgver"
    gcc -o razzshell razzshell.c -lreadline -ldl
}

package() {
    cd "$srcdir/$pkgname-$pkgver"
    install -Dm755 razzshell "$pkgdir/usr/bin/razzshell"
    install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md"
    install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
