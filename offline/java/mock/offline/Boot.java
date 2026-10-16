package mock.offline;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.util.Log;

/**
 * Starts the bundled mock server when the app process starts. A content provider is created before the
 * Application and before any activity, so the server is listening by the time the game makes its first request,
 * and adding one only takes a manifest entry (no change to the game's own code).
 */
public class Boot extends ContentProvider {
    @Override
    public boolean onCreate() {
        try {
            HttpServer.start(getContext());
        } catch (Throwable t) {
            Log.e(HttpServer.TAG, "server failed to start", t);
        }
        return true;
    }

    @Override public Cursor query(Uri u, String[] p, String s, String[] a, String o) { return null; }
    @Override public String getType(Uri u) { return null; }
    @Override public Uri insert(Uri u, ContentValues v) { return null; }
    @Override public int delete(Uri u, String s, String[] a) { return 0; }
    @Override public int update(Uri u, ContentValues v, String s, String[] a) { return 0; }
}
