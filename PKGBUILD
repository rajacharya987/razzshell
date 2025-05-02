# Maintainer: Raj Acharya <razzacharya6@gmail.com>
pkgname=razzshell
pkgver=1.0.2
pkgrel=1
pkgdesc="A custom Unix shell with unique features"
arch=('x86_64')
url="https://github.com/rajacharya987/razzshell"
license=('MIT')
depends=('readline')
source=("https://github.com/rajacharya987/razzshell/releases/download/v1.0.2/razzshell-1.0.2.tar.gz")
sha256sums=('654bdfaa758be6a3e0c844ea9516e9a47b4edbcd5a2b2e9232c6616489892e40')

# Correct build function to handle extraction and directory change
build() {
    cd "$srcdir"
    tar -xzf "$srcdir/razzshell-1.0.2.tar.gz" -C "$srcdir"
    cd "$srcdir/razzshell-1.0.2"  # Ensure we're in the correct directory
    gcc -o razzshell razzshell.c -lreadline -ldl
}

package() {
    cd "$srcdir/razzshell-1.0.2"  # Ensure we're in the correct directory
    install -Dm755 razzshell "$pkgdir/usr/bin/razzshell"
    install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md"
    install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
