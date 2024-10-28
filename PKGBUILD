# Maintainer: Raj Acharya <razzacharya6@gmail.com>
pkgname=razzshell
pkgver=1.0.0
pkgrel=1
pkgdesc="A costom Unix shell with unique features"
arch=('x86_64')
url="https://github.com/rajacharya987/razzshell.git"
license=('MIT')
depends=('readline')
source=("$pkgname-$pkgver.tar.gz::$url/archive/v$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cd "$srcdir/$pkgname-$pkgver"
    gcc -o razzshell razzshell.c -lreadline
}

package() {
    cd "$srcdir/$pkgname-$pkgver"
    install -Dm755 razzshell "$pkgdir/usr/bin/razzshell"
    install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md"
    install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
